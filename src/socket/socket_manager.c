#include "socket_manager.h"
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
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)
/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */

void read_write_loop(const int sfd){

    int nfds = 2;
    struct pollfd *pfds;
    pfds = malloc(sizeof(struct pollfd)*nfds);


    while(1){

        pfds[0].fd = STDIN_FILENO;
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
        char buf[1024];
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