#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include "packet/packet.h"
#include "logs/log.h"
#include "socket/socket_manager.h"
#include "buffer/buffer.h"

#define BUFF_LEN 
#define ACK_SIZE 10
#define MAX_SEQNUM 256

int window = 1;
//receiver window
int window_r = 31;
int fd = STDIN_FILENO;
int seqnum = 0;
int lastack = -1;
int nb_sended = 0;
uint8_t nextSeqnum;
struct timeval tv;

// Stats
int stats_data_sent = 0;
int stats_ack_received = 0;
int stats_nack_received = 0;
int stats_min_rtt = 2147483647;
int stats_max_rtt = 0;
int rtt[256];

int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-f filename] [-s stats_filename] receiver_ip receiver_port", prog_name);
    return EXIT_FAILURE;
}

int is_in_window(int seqnum_to_test){
    if(seqnum_to_test < seqnum){
        if(seqnum_to_test + MAX_SEQNUM - seqnum < window)
            return 1;
        return 0;
    }else{
        if(seqnum_to_test - seqnum <= window)
            return 1;
        return 0;
    }
}

// int is_valid(int seqnum_to_test){
//     if(seqnum_to_test < window){
//         if(nb_sended % MAX_SEQNUM < window){

//         }
//     }
// }


pkt_t* read_file(buffer_t* buffer, int fd){
    if(!buffer){
        ERROR("Buffer is null");
        return NULL;
    }
    if((int)buffer->size >= window){
        //ERROR("Buffer is full");
        return NULL;
    }

    time_t now = time(NULL);

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
    err = pkt_set_timestamp(pkt,(uint32_t)now);
    err = pkt_set_seqnum(pkt,seqnum);
    if(readed == 0){
        err = pkt_set_length(pkt,0);
    }else{
        err = pkt_set_payload(pkt,paylaod,readed);
        seqnum = (seqnum + 1) % MAX_SEQNUM;
        nb_sended++;
    }
    
    if(err){
        ERROR("Can't set data to the packet : %d",err);
        pkt_del(pkt);
        return NULL;
    }


    return pkt;
}

// int read_file_to_buffer(buffer_t* buffer, int fd){
//     if(!buffer){
//         ERROR("Buffer is null");
//         return 0;
//     }

//     if(buffer->size > window){
//         ERROR("Buffer is full");
//         return -1;
//     }

//     int nbPackets = 0;
//     ssize_t datas;

//     while(buffer->size < window && datas > 0 ){
//         char payload[MAX_PAYLOAD_SIZE];
//         datas = read(fd,payload,MAX_PAYLOAD_SIZE);
//         if(datas == -1){
//             ERROR("Can't read data in file");
//             return 0;
//         }

//         pkt_t* pkt = pkt_new();
//         if(!pkt){
//             ERROR("Can't create packet");
//             return 0;
//         }

//         pkt_status_code err;
//         err = pkt_set_type(pkt,PTYPE_DATA);
//         err = pkt_set_tr(pkt,0);
//         if(datas == 0){
//             err = pkt_set_seqnum(pkt,seqnum);
//             err = pkt_set_length(pkt,0);
//         }else{
//             err = pkt_set_seqnum(pkt,(seqnum++ % 256));
//             err = pkt_set_payload(pkt,payload,datas);
//         }        
//         if(err){
//             ERROR("Can't set data to the packet : %d",err);
//             pkt_del(pkt);
//             return 0;
//         }

//         buffer_enqueue(buffer,pkt);
//         nbPackets++;
//     }
//     return nbPackets;
// }


void read_write_loop_sender(const int sfd, const int fdIn){

    int nfds = 2;
    struct pollfd *pfds;
    pfds = malloc(sizeof(struct pollfd)*nfds);
    int last_ack_to_receive = -1;

    int error;
    pkt_t* pkt;
    pkt_t* pkt_to;
    char buf[MAX_PKT_SIZE];
    char buf_ack[ACK_SIZE];
    size_t len;
    ssize_t readed;
    buffer_t* buffer = buffer_init();

    pkt_status_code st;        
    int EOF_ACKED = 0;
    int ready = -1;
    int nb_last_ack_to = 0;

    while(EOF_ACKED != 2 && ready != 0){

        pfds[0].fd = fdIn;
        pfds[0].events = POLLIN;
        pfds[1].fd = sfd;
        pfds[1].events = POLLIN;

        
        ready = poll(pfds,nfds,10 * 1000);
        if(ready == -1){
            ERROR("Poll error");
            return;
        }
        if(ready == 0){
            ERROR("Connection timed out");
        }


        if(pfds[0].revents != 0 && pfds[0].revents & POLLIN ){
            
            pkt_to = look_for_timedout_packet(buffer);
            if(pkt_to){
                if(pkt_get_seqnum(pkt_to) == last_ack_to_receive){
                    nb_last_ack_to++;
                    if(nb_last_ack_to == 4){
                        ERROR("EOF packet timed out 4 times, no response. Connection finished");
                        break;
                    }
                }
                st = pkt_encode(pkt_to,buf,&len);
                    
                if(st){
                    pkt_del(pkt_to);
                    ERROR("Error while encoding packet : %d",st);
                    continue;
                }
                
                error = send(sfd,buf,len,0);
                if(error == -1){
                    ERROR("ERROR while sending timed out packet : %d",pkt_get_timestamp(pkt_to));
                }
                rtt[pkt_get_seqnum(pkt_to)] = time(NULL);
                stats_data_sent++;
                ERROR("Packet timed out [%d] re sended",pkt_get_seqnum(pkt_to));
            }
            
            pkt = read_file(buffer,fdIn);
            if(pkt && last_ack_to_receive == -1){
                if(pkt_get_length(pkt) == 0 && pkt_get_seqnum(pkt) == seqnum){
                    last_ack_to_receive = seqnum;
                    ERROR("last_ack_to_receive = %d\n",seqnum);
                }
                buffer_enqueue(buffer,pkt);
                ERROR("add : ");fflush(stdout);buffer_print(buffer);
                // Test truncated
                /*if(seqnum % 5 != 0){
                    pkt_set_tr(pkt,1);
                }*/
                st = pkt_encode(pkt,buf,&len);
                
                if(st){
                    pkt_del(pkt);
                    ERROR("Error while encoding packet : %d",st);
                    continue;
                }
                rtt[pkt_get_seqnum(pkt)] = time(NULL);
                stats_data_sent++;
                
                ERROR("Send the packet [%d]",pkt_get_seqnum(pkt));
                //ERROR("don't send packed %d : %d",seqnum,seqnum%5==0);
                //if(seqnum % 5 != 0) 
                error = send(sfd,buf,len,0);
                ERROR("%d bytes sended\n",error);
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
            //Black magic condition
            if(pkt_get_seqnum(pkt) > nb_sended+1 % MAX_SEQNUM){
                //buffer_get_pkt(buffer,pkt_get_seqnum(pkt)-(1-pkt_get_tr(pkt)) ) == NULL && (last_ack_to_receive == pkt_get_seqnum(pkt))){
                ERROR("Ack [%d] invalid",pkt_get_seqnum(pkt));
                pkt_del(pkt);
                continue;
            }
            if(pkt_get_type(pkt) == PTYPE_NACK){
                ERROR("Packed [%d] was truncated",pkt_get_seqnum(pkt));
                //pkt_del(pkt);
                // continue;
                pkt_t* resend = pkt_new();
                resend = buffer_get_pkt(buffer,pkt_get_seqnum(pkt));
//                pkt_print(resend);
                st = pkt_encode(resend,buf,&len);  
                if(st){
                    pkt_del(pkt);
                    ERROR("Error while encoding packet : %d",st);
                    continue;
                }
                
                ERROR("Send directly the truncated [%d] packet\n",seqnum);
                error = send(sfd,buf,len,0);
                if(error == -1){
                    ERROR("ERROR while sending packet : %d",seqnum);
                }
                rtt[pkt_get_seqnum(pkt)] = time(NULL);
                stats_data_sent++;
                stats_nack_received++;
            }
            else{
                window = pkt_get_window(pkt);
                int ack_received = pkt_get_seqnum(pkt);

                error = buffer_remove_acked(buffer,ack_received);
                ERROR("ack %d received, %d pkt removed",ack_received,error);
            
                ERROR("remove : ");fflush(stdout);
                buffer_print(buffer);
                fprintf(stderr,"\n");
                stats_ack_received++;
                int rtt_total = time(NULL) - rtt[pkt_get_seqnum(pkt)];
                if(rtt_total > stats_max_rtt) stats_max_rtt = rtt_total;
                if(rtt_total < stats_min_rtt) stats_min_rtt = rtt_total;
                
                // ERROR("Last received %d vs received %d",lastack,ack_received);
                if(error > 0 || last_ack_to_receive != -1){
                lastack = ack_received;
                    if(lastack == last_ack_to_receive){
                        ERROR("Reached the EOF");
                        pkt_del(pkt);
                        EOF_ACKED++;
                    }
                }
            }
        }
    }
}

int write_stats_in_file(char* filename){
    FILE *file = fopen(filename,"w");
    if(file == NULL){
        return -1;
    }

    fprintf(file,"data_sent:%d\n",stats_data_sent);
    fprintf(file,"ack_received:%d\n",stats_ack_received);
    fprintf(file,"nack_received:%d\n",stats_nack_received);
    fprintf(file,"min_rtt:%d\n",stats_min_rtt);
    fprintf(file,"max_rtt:%d\n",stats_max_rtt);
    return 0;
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

    int sock;

    const char* err = real_address(receiver_ip,&receiver_addr);
    if(err){
        ERROR("Could not resolve hostname %s : %d",receiver_ip,receiver_port);
        exit(EXIT_FAILURE);
    }

    sock = create_socket(NULL,-1,&receiver_addr,receiver_port);
    if(sock < 0){
        ERROR("Failed to create socket at %s : %d",receiver_ip,receiver_port);
        exit(EXIT_FAILURE);
    }

    read_write_loop_sender(sock,fd);

    close(sock);
    close(fd);

    read_write_loop_sender(sock,fd);

    write_stats_in_file(stats_filename);
    /*buffer_t* buffer = buffer_init();
    int nb = read_file_to_buffer(buffer,fd);
    printf("%d",nb);
    buffer_print(buffer,0);*/
    return EXIT_SUCCESS;
}
