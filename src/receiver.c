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

#include "packet/packet.h"
#include "socket/socket_manager.h"
#include "buffer/buffer.h"
#include "logs/log.h"

#define MAX_PKT_SIZE 512 + 16
#define MAX_SEQNUM 255
#define ACK_SIZE 10

int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-s stats_filename] listen_ip listen_port", prog_name);
    return EXIT_FAILURE;
}

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
    
    ssize_t sended = send(sfd,buf,len,0);
    if(sended == -1){
        ERROR("Can't send ack packet");
        pkt_del(ack);
        return -1;
    }

    printf("\n\nack %d sended\n\n",next_seqnum);

    return 0;
}

void read_write_loop_receiver(const int sfd,const int fdOut){

    int nfds = 1;
    struct pollfd *pfds;
    pfds = malloc(sizeof(struct pollfd)*nfds);
    buffer_t* buffer = buffer_init();
    int lastack_to_send = -2;

    while((next_seqnum-1) != lastack_to_send){

        pfds[0].fd = sfd;
        pfds[0].events = POLLIN;

        int ready;
        
        ready = poll(pfds,nfds,-1);
        if(ready == -1){
            ERROR("Poll error");
            return;
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

                if(pkt_get_type(pkt) == PTYPE_DATA 
                    && pkt_get_length(pkt) == 0
                    && pkt_get_seqnum(pkt) == next_seqnum){
                    ERROR("EOF for sender");
                    lastack_to_send = pkt_get_seqnum(pkt);
                }else{
                    //error = write_buffer_to_file(buffer,sfd,fdOut);
                    error = write(fdOut,buf,s);
                    if(error == -1){
                        ERROR("Error while writing");
                        //return;
                    }
                }
                next_seqnum = pkt_get_seqnum(pkt) + 1;
                window++;
                send_ack(sfd,next_seqnum);

            }


            
        }
        
    }
}


// gcc receiver.c -o receiver
// ./receiver ipv6 port
// 2a02:2788:1a4:552:eee9:bbda:b6d0:4e08
int main(int argc, char **argv) {
    int opt;

    char *stats_filename = NULL;
    char *listen_ip = NULL;
    char *listen_port_err;
    uint16_t listen_port;

    while ((opt = getopt(argc, argv, "s:h")) != -1) {
        switch (opt) {
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

    ASSERT(1 == 1); // Try to change it to see what happens when it fails
    DEBUG_DUMP("Some bytes", 11); // You can use it with any pointer type

    // This is not an error per-se.
    ERROR("Receiver has following arguments: stats_filename is %s, listen_ip is %s, listen_port is %u",
        stats_filename, listen_ip, listen_port);

    struct sockaddr_in6 listener_addr;


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

    read_write_loop_receiver(sock,STDOUT_FILENO);

    return EXIT_SUCCESS;
}
