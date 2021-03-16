#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include "packet/packet.h"
#include "logs/log.h"
#include "socket/socket_manager.h"
#include "buffer/buffer.h"

#define BUFF_LEN 
#define ACK_SIZE 10

int window = 31;
//receiver window
int window_r = 31;
int fd = STDIN_FILENO;
int seqnum = 0;
int eof = 0;
int lastack = -1;
uint8_t nextSeqnum;


int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-f filename] [-s stats_filename] receiver_ip receiver_port", prog_name);
    return EXIT_FAILURE;
}

pkt_t* read_file(buffer_t* buffer, int fd){
    if(!buffer){
        ERROR("Buffer is null");
        return NULL;
    }
    // if(buffer->size > WINDOW){
    //     ERROR("Buffer is full");
    //     return NULL;
    // }

    char paylaod[MAX_PAYLOAD_SIZE];
    ssize_t readed = read(fd,paylaod,MAX_PAYLOAD_SIZE);

    if(readed == -1){
        ERROR("Can't read data in file");
        return NULL;
    }
    pkt_t* pkt = pkt_new();
    if(!pkt){
        ERROR("Can't create packet");
        return NULL;
    }
    pkt_status_code err = pkt_set_type(pkt,PTYPE_DATA);
    err = pkt_set_tr(pkt,0);
    err = pkt_set_window(pkt,window);
    if(readed == 0){
        err = pkt_set_seqnum(pkt,seqnum);
        err = pkt_set_length(pkt,0);
    }else{
        err = pkt_set_seqnum(pkt,(seqnum++ % 256));
        err = pkt_set_payload(pkt,paylaod,readed);
    }
    
    if(err){
        ERROR("Can't set data to the packet : %d",err);
        pkt_del(pkt);
        return NULL;
    }

    return pkt;
}

int read_file_to_buffer(buffer_t* buffer, int fd){
    if(!buffer){
        ERROR("Buffer is null");
        return 0;
    }

    int nbPackets = 0;
    ssize_t datas;

    while(datas > 0){
        char payload[MAX_PAYLOAD_SIZE];
        datas = read(fd,payload,MAX_PAYLOAD_SIZE);
        if(datas == -1){
            ERROR("Can't read data in file");
            return 0;
        }

        pkt_t* pkt = pkt_new();
        if(!pkt){
            ERROR("Can't create packet");
            return 0;
        }

        pkt_status_code err;
        err = pkt_set_type(pkt,PTYPE_DATA);
        err = pkt_set_tr(pkt,0);
        err = pkt_set_window(pkt,window);
        err = pkt_set_seqnum(pkt,(seqnum++ % 256));
        err = pkt_set_payload(pkt,payload,datas);
        if(err){
            ERROR("Can't set data to the packet : %d",err);
            pkt_del(pkt);
            return 0;
        }

        buffer_enqueue(buffer,pkt);
        nbPackets++;
    }
    return nbPackets;
}


void read_write_loop_sender(const int sfd, const int fdIn){

    int nfds = 2;
    struct pollfd *pfds;
    pfds = malloc(sizeof(struct pollfd)*nfds);
    int last_ack_to_receive = -1;

    int error;
    pkt_t* pkt;
    char buf[MAX_PKT_SIZE];
    char buf_ack[ACK_SIZE];
    size_t len;
    ssize_t readed;
    buffer_t* buffer = buffer_init();

    pkt_status_code st;        
    while(!eof){

        pfds[0].fd = fdIn;
        pfds[0].events = POLLIN;
        pfds[1].fd = sfd;
        pfds[1].events = POLLIN;

        int ready;
        
        ready = poll(pfds,nfds,-1);
        if(ready == -1){
            ERROR("Poll error");
            return;
        }

        
        if(pfds[0].revents != 0 && pfds[0].revents & POLLIN && last_ack_to_receive == -1){

            pkt = read_file(buffer,fdIn);
            if(pkt){
                if(pkt_get_length(pkt) == 0 && pkt_get_seqnum(pkt) == seqnum){
                    last_ack_to_receive = seqnum;
                    //printf("last_ack_to_receive = %d\n",seqnum);
                }
                buffer_enqueue(buffer,pkt);
                //printf("add : ");fflush(stdout);buffer_print(buffer,buffer->size);
                st = pkt_encode(pkt,buf,&len);
                
                if(st){
                    pkt_del(pkt);
                    ERROR("Error while encoding packet : %d",st);
                    continue;
                }
                
                ERROR("send the %d packet\n",seqnum);
                error = send(sfd,buf,len,0);
                if(error == -1){
                    ERROR("ERROR while sending packet : %d",seqnum);
                }
            
            }
        }
        if(pfds[1].revents != 0 && pfds[1].revents & POLLIN){
            readed = read(sfd,buf_ack,ACK_SIZE);
            if(readed == -1){
                ERROR("Error while reading ack");
                continue;
            }
            
            pkt = pkt_new();
            if(!pkt){
                ERROR("read_write_loop_sender : Out of memory for create a packet ");
                pkt_del(pkt);
                continue;
            }

            error = pkt_decode(buf_ack,readed,pkt);
            if(error || pkt_get_type(pkt) == PTYPE_DATA){
                ERROR("read_write_loop_sender : Error while decoding ack packet");
                pkt_del(pkt);
                continue;
            }
            window_r = pkt_get_window(pkt);
            int ack_received = pkt_get_seqnum(pkt);

            error = buffer_remove_acked(buffer,ack_received);
            ERROR("ack %d received, %d pkt removed\n",ack_received,error);
           
            //printf("remove : ");fflush(stdout);
            buffer_print(buffer,buffer->size);
        
            if(error > 0){
                lastack = ack_received;
                if(lastack == last_ack_to_receive){
                    ERROR("Reached the EOF");
                    pkt_del(pkt);
                    eof = 1;
                }
            }

        }
    }
}


// gcc sender.c packet/packet.c logs/log.c socket/socket_manager.c buffer/buffer.c -o sender -lz
// gcc sender.c -o sender
// ./sender ipv6 port
int main(int argc, char **argv) {
    int opt;

    char *filename = NULL;
    char *stats_filename = NULL;
    char *receiver_ip = NULL;
    char *receiver_port_err;
    uint16_t receiver_port;

    while ((opt = getopt(argc, argv, "f:s:h")) != -1) {
        switch (opt) {
        case 'f':
            filename = optarg;
            break;
        case 'h':
            return print_usage(argv[0]);
        case 's':
            stats_filename = optarg;
            break;
        default:
            return print_usage(argv[0]);
        }
    }

    if (optind + 2 != argc) {
        ERROR("Unexpected number of positional arguments");
        return print_usage(argv[0]);
    }

    receiver_ip = argv[optind];
    receiver_port = (uint16_t) strtol(argv[optind + 1], &receiver_port_err, 10);
    if (*receiver_port_err != '\0') {
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

    ASSERT(1 == 1); // Try to change it to see what happens when it fails
    DEBUG_DUMP("Some bytes", 12); // You can use it with any pointer type
    
    ERROR("Sender has following arguments: filename is %s, stats_filename is %s, receiver_ip is %s, receiver_port is %u",
        filename, stats_filename, receiver_ip, receiver_port);

    DEBUG("You can only see me if %s", "you built me using `make debug`");

    struct sockaddr_in6 receiver_addr;

    if(filename){
        fd = open(filename,O_RDWR);
        if(fd < 0){
            ERROR("Can't open file %s",filename);
        }
    }

    const char* err = real_address(receiver_ip,&receiver_addr);
    if(err){
        ERROR("Could not resolve hostname %s : %d",receiver_ip,receiver_port);
        exit(EXIT_FAILURE);
    }

    int sock = create_socket(NULL,-1,&receiver_addr,receiver_port);
    if(sock < 0){
        ERROR("Failed to create socket at %s : %d",receiver_ip,receiver_port);
        exit(EXIT_FAILURE);
    }


    // int cpt = 0;
    // while(!eof){
    //     pkt = read_file(buffer,fd);
    //     if(pkt){
    //         buffer_enqueue(buffer,pkt);
    //         st = pkt_encode(pkt,buf,&len);
    //         if(st){
    //            ERROR("Error while encoding packet : %d",st); 
    //         }else{
    //             send(sock,buf,len,0);
    //             cpt++;
    //         }
    //     }
    // }
    // printf("%d packet sended\n",cpt);

    read_write_loop_sender(sock,fd);
    /*buffer_t* buffer = buffer_init();
    int nb = read_file_to_buffer(buffer,fd);
    printf("%d",nb);
    buffer_print(buffer,0);*/
    return EXIT_SUCCESS;
}
