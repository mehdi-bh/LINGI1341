#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>

#include "packet/packet.h"
#include "socket/socket_manager.h"
#include "buffer/buffer.h"
#include "logs/log.h"

#define MAX_PKT_SIZE 512 + 16
#define MAX_SEQNUM 256
#define ACK_SIZE 10

int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-s stats_filename] listen_ip listen_port", prog_name);
    return EXIT_FAILURE;
}

int window = 31;
int last_acked = 0;
int last_writed = -1;
int nb_writed = 0;
int potentialEOF = 0;
int lastack_to_send = -2;

int fd = STDOUT_FILENO;


int is_in_window(int seqnum){
    if(seqnum == last_writed) return 0;
    if(seqnum < last_writed){
        if(seqnum + MAX_SEQNUM - last_writed <= window)
            return 1;
        return 0;
    }else{
        if(seqnum - last_writed < window)
            return 1;
        return 0;
    }
}

int get_first_oos_seqnum(buffer_t* buffer){
    if(buffer->size == 0){
        return 0;
    }
    int seq = last_acked;
    node_t* current = buffer->first;
    while(current != NULL){
        if(seq == pkt_get_seqnum(current->pkt)-1){
            seq += 1;
        }else{
            return seq+1;
        }
        current = current->next;
    }
    return seq + 1;
}

int write_buffer_to_file(buffer_t* buffer, const int fdOut){
    node_t* current = buffer->first;
    pkt_t* pkt;
    buffer_print(buffer);
    while(current != NULL && pkt_get_seqnum(current->pkt) == (last_writed+1)%MAX_SEQNUM ){
        pkt = current->pkt;
        ssize_t writed = write(fdOut,pkt_get_payload(pkt),pkt_get_length(pkt));
        if(writed == -1){
            ERROR("Receiver can't write to file");
            return -1;
        }
        ERROR("packet %d writed",pkt_get_seqnum(pkt));
        last_writed = pkt_get_seqnum(pkt);

        current = current->next;
        pkt = buffer_remove(buffer,pkt_get_seqnum(pkt));
        nb_writed++;
    }
    return 1;
}

int send_ack(const int sfd,int seqnum, uint8_t type){
    pkt_t* ack = pkt_new();
    if(!ack){
        ERROR("Can't create ack packet");
        return -1;
    }
    pkt_set_type(ack,type);
    pkt_set_seqnum(ack,seqnum);
    pkt_set_tr(ack,0);
    pkt_set_window(ack,window);
    pkt_set_timestamp(ack,(uint32_t) time(NULL));
    pkt_set_payload(ack,NULL,0);

    char buf[ACK_SIZE];
    size_t len;
    pkt_status_code st = pkt_encode(ack,buf,&len);
    if(st != PKT_OK){
        ERROR("Can't encode ack packet : %d",st);
        pkt_del(ack);
        return -1;
    }
    
    ssize_t sended = send(sfd,buf,len,0);
    if(sended == -1){
        ERROR("Can't send ack packet");
        pkt_del(ack);
        return -1;
    }
    if(type == PTYPE_ACK){
        last_acked = seqnum;
        ERROR("ack %d sended\n",seqnum);
    }
    else{
        ERROR("nack %d sended\n",seqnum);
    }


    return 0;
}



void read_write_loop_receiver(const int sfd,const int fdOut){

    int nfds = 1;
    struct pollfd *pfds;
    pfds = malloc(sizeof(struct pollfd)*nfds);
    buffer_t* buffer = buffer_init();
    int ready =-1;
        srand(time(NULL));

    while(last_acked != lastack_to_send 
        && ready != 0
        //&& potentialEOF != 2
    ){

        pfds[0].fd = sfd;
        pfds[0].events = POLLIN;

        
        ready = poll(pfds,nfds,10 * 1000);
        if(ready == -1){
            ERROR("Poll error");
            return;
        }
        if(ready == 0){
            ERROR("Connection timed out, consider finished");
        }

        int error;
        ssize_t s;
        pkt_t* pkt = pkt_new();
        pkt_status_code st;
        char buf[MAX_PKT_SIZE];
        if(pfds[0].revents != 0 && pfds[0].revents & POLLIN){
            s = read(pfds[0].fd,buf,MAX_PKT_SIZE);
            ERROR("%ld bytes readed",s);
            if(s == -1){
                ERROR("Can't read socket");continue;
            }
            st = pkt_decode(buf,s,pkt);
            if(st != PKT_OK || pkt_get_type(pkt) != PTYPE_DATA){
                ERROR("Packet incorrect : %d",st);pkt_del(pkt);continue;
            }
            ERROR("Packet [%d] received nb writed [%d]",pkt_get_seqnum(pkt),nb_writed);
            if(is_in_buffer(buffer,pkt_get_seqnum(pkt))){ 
                ERROR("Duplicate packet [%d]",pkt_get_seqnum(pkt));
                continue;
            }
            if(is_in_window(pkt_get_seqnum(pkt))){
                
                if(rand() % 20 == 0)
                    pkt_set_tr(pkt,1);

                if(pkt_get_tr(pkt) == 1){
                    send_ack(sfd,pkt_get_seqnum(pkt), PTYPE_NACK);
                }
                else if(pkt_get_type(pkt) == PTYPE_DATA 
                        && pkt_get_length(pkt) == 0
                ){

                    //if(pkt_get_seqnum(pkt) == potentialEOF){
                    if(pkt_get_seqnum(pkt) == (nb_writed) % MAX_SEQNUM){
                        ERROR("EOF for sender");
                        lastack_to_send = pkt_get_seqnum(pkt);
                    }
                    //}
                    // ERROR("Potential EOF [%d]",potentialEOF);
                    // potentialEOF = pkt_get_seqnum(pkt) ;
                }
                else{
                    error = buffer_enqueue(buffer,pkt);
                    if(error == -1){
                        ERROR("Out of memory while adding to buffer");return;
                    }

                    error = write_buffer_to_file(buffer,fdOut);
                    if(error == -1){
                        ERROR("Error while writing");
                        continue;
                    }
                }
                send_ack(sfd,(last_writed + 1) % MAX_SEQNUM, PTYPE_ACK);
            }
            else{
                ERROR("Packet not in window [%d]",pkt_get_seqnum(pkt));
            }
        }
    }
}


// gcc receiver.c -o receiver
// ./receiver ipv6 port
// 2a02:2788:1a4:552:eee9:bbda:b6d0:4e08
int main(int argc, char **argv) {
    int opt;

    char *filename = NULL;
    char *stats_filename = NULL;
    char *listen_ip = NULL;
    char *listen_port_err;
    uint16_t listen_port;

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

    listen_ip = argv[optind];
    listen_port = (uint16_t) strtol(argv[optind + 1], &listen_port_err, 10);
    if (*listen_port_err != '\0') {
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

    ERROR("Receiver has following arguments: stats_filename is %s, listen_ip is %s, listen_port is %u",
        stats_filename, listen_ip, listen_port);

    struct sockaddr_in6 listener_addr;

    if(filename){
        fd = open(filename,O_WRONLY | O_RDONLY | O_CREAT,0644);
        if(fd < 0){
            ERROR("Can't open file %s",filename);
        }
    }

    const char* err = real_address(listen_ip,&listener_addr);
    if(err){
        ERROR("Could not resolve hostname %s : %d",listen_ip,listen_port);
        exit(EXIT_FAILURE);
    }

    int sock = create_socket(&listener_addr,listen_port,NULL,-1);
    if(sock < 0){
        ERROR("Failed to create socket at %s : %d",listen_ip,listen_port);
        exit(EXIT_FAILURE);
    }

    int error = wait_for_client(sock);
    if(error < 0){
        ERROR("Can't connect to client");
        close(sock);
        return -1;
    }

    read_write_loop_receiver(sock,fd);

    return EXIT_SUCCESS;
}
