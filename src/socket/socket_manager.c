#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "socket_manager.h"
#include "../packet/packet.h"
#include "../buffer/buffer.h"
#include "../logs/log.h"

#define MAX_PKT_SIZE 512 + 16
#define MAX_SEQNUM 255
#define ACK_SIZE 10
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)

int window = 31;
int next_seqnum = 0;

int is_in_window(int seqnum){
    if(seqnum < (next_seqnum + window) % MAX_SEQNUM){
        return 1;
    }
    return 0;
}

// int write_buffer_to_file(buffer_t* buffer,const int sfd, const int fdOut){
//     node_t* current = buffer->first;
//     pkt_t* pkt = current->pkt;
//     while(current != NULL){
//         ssize_t writed = write(sfd,pkt_get_payload(pkt),pkt_get_length(pkt));
//         if(writed == -1){
//             ERROR("Receiver can't write to file");
//             return -1;
//         }

        
//     }
// }

int send_ack(const int sfd,int seqnum){
    pkt_t* ack = pkt_new();
    if(!ack){
        ERROR("Can't create ack packet");
        return -1;
    }
    pkt_set_type(ack,PTYPE_ACK);
    pkt_set_seqnum(ack,seqnum);
    pkt_set_tr(ack,0);
    pkt_set_payload(ack,NULL,0);
    pkt_set_window(ack,window);

    char buf[ACK_SIZE];
    size_t len;
    pkt_status_code st = pkt_encode(ack,buf,&len);
    if(st != PKT_OK){
        ERROR("Can't encode ack packet : %d",st);
        pkt_del(ack);
        return -1;
    }

    ssize_t sended = (sfd,buf,len);
    if(sended == -1){
        ERROR("Can't send ack packet");
        pkt_del(ack);
        return -1;
    }

    return 0;
}

/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop_receiver(const int sfd,const int fdOut){

    int nfds = 1;
    struct pollfd *pfds;
    pfds = malloc(sizeof(struct pollfd)*nfds);
    buffer_t* buffer = buffer_init();

    while(1){

        pfds[0].fd = sfd;
        pfds[0].events = POLLIN;

        int ready;
        
        ready = poll(pfds,nfds,-1);
        if(ready == -1){
            errExit("open");
        }

        int error;
        ssize_t s;
        pkt_t* pkt = pkt_new();
        pkt_status_code st;
        char buf[MAX_PKT_SIZE];
        if(pfds[0].revents != 0 && pfds[0].revents & POLLIN){
            s = read(pfds[0].fd,buf,MAX_PKT_SIZE);
            if(s == -1){
                ERROR("Can't read socket");return;
            }
            st = pkt_decode(buf,s,pkt);
            if(st != PKT_OK || pkt_get_type(pkt) != PTYPE_DATA){
                ERROR("Packet incorrect : %d",st);pkt_del(pkt);return;
            }
            
            if(is_in_window(pkt_get_seqnum(pkt))){
                error = buffer_enqueue(buffer,pkt);
                if(error == -1){
                    ERROR("Out of memory while adding to buffer");return;
                }
                window--;

                //error = write_buffer_to_file(buffer,sfd,fdOut);
                error = write(fdOut,buf,s);
                if(error == -1){
                    ERROR("Error while writing");
                    //return;
                }
                window++;

                send_ack(sfd,pkt_get_seqnum(pkt));
            }


            
        }
        
    }
}

void read_write_loop_sender(const int sfd, const int fdIn){

    int nfds = 2;
    struct pollfd *pfds;
    pfds = malloc(sizeof(struct pollfd)*nfds);


    while(1){

        pfds[0].fd = fdIn;
        pfds[0].events = POLLIN;
        pfds[1].fd = sfd;
        pfds[1].events = POLLIN;

        int ready;
        
        ready = poll(pfds,nfds,-1);
        if(ready == -1){
            errExit("open");
        }

        int error;
        ssize_t s;
        char buf[MAX_PKT_SIZE];
        
        if(pfds[0].revents != 0 && pfds[0].revents & POLLIN){
            s = read(pfds[0].fd,buf,sizeof(buf));
            if(s == -1)
                errExit("read");

            error = write(sfd,buf,s);
            if(error == -1){
                return;
            }
        }
        if(pfds[1].revents != 0 && pfds[1].revents & POLLIN){
            s = read(pfds[1].fd,buf,sizeof(buf));
            if(s == -1)
                errExit("read2");
            error = write(STDOUT_FILENO,buf,s);
            if(error == -1)
                return;
        }
    }
}

const char * real_address(const char *address, struct sockaddr_in6 *rval){
    struct addrinfo hint;
    struct addrinfo* result;

    memset(&hint,0,sizeof(hint));

    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_family = AF_INET6;
    hint.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(address,NULL,&hint,&result);
    if(s!=0){
        return gai_strerror(s);
    }

    memcpy(rval, result->ai_addr, sizeof(struct sockaddr_in6)); // copying the values to rval
    freeaddrinfo(result);
    return NULL;
}


int create_socket(struct sockaddr_in6 *source_addr,
                 int src_port,
                 struct sockaddr_in6 *dest_addr,
                 int dst_port){
    
    int sock = socket(AF_INET6,SOCK_DGRAM,IPPROTO_UDP);
    if(sock == -1){
        fprintf(stderr, "socket returned a -1 descriptor\n");
        return -1;
    }

    if(source_addr != NULL){
        if(src_port > 0){ 
            source_addr->sin6_port = htons(src_port);
        }

        if(bind(sock,(struct sockaddr*)source_addr,sizeof(struct sockaddr_in6)) != 0){
            fprintf(stderr, "bind returned -1 \n");
            return -1;
        }
    }
    if(dest_addr != NULL){
        if(dst_port > 0){
            dest_addr->sin6_port = htons(dst_port);
        }
        if(connect(sock,(struct sockaddr*)dest_addr,sizeof(struct sockaddr_in6)) != 0){
            fprintf(stderr, "connect returned -1\n");
            return -1;
        }
    }


    return sock;
}