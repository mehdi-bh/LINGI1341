#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
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

#define errExit(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

int wait_for_client(int sfd){
    char buffer[1024];
    
    struct sockaddr_in6 sock;
    socklen_t len = sizeof(sock);

    int nread = recvfrom(sfd, buffer, sizeof(char) * 1024, MSG_PEEK, (struct sockaddr*) &sock, &len);
    if(nread == -1){
        fprintf(stderr, "Error in recvfrom() \n");
        return -1;
    }

    int done = connect(sfd, (struct sockaddr*) &sock, (int) len);
    if(done == -1){
        fprintf(stderr, "Error in connect() \n");
        return -1;
    }

    return 0;
}

const char * real_address(const char *address, struct sockaddr_in6 *rval){
    struct addrinfo hint;
    struct addrinfo* result;

    memset(&hint, 0, sizeof(hint));

    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_family = AF_INET6;
    hint.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(address,NULL,&hint,&result);
    if(s != 0){
        return gai_strerror(s);
    }

    memcpy(rval, result->ai_addr, sizeof(struct sockaddr_in6));
    freeaddrinfo(result);
    return NULL;
}


int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port){
    int sock = socket(AF_INET6,SOCK_DGRAM,IPPROTO_UDP);
    if(sock == -1){
        fprintf(stderr, "Error in socket() \n");
        return -1;
    }

    if(source_addr != NULL){
        if(src_port > 0){ 
            source_addr->sin6_port = htons(src_port);
        }

        if(bind(sock,(struct sockaddr*)source_addr,sizeof(struct sockaddr_in6)) != 0){
            fprintf(stderr, "Error in bind() \n");
            return -1;
        }
    }

    if(dest_addr != NULL){
        if(dst_port > 0){
            dest_addr->sin6_port = htons(dst_port);
        }
        if(connect(sock,(struct sockaddr*)dest_addr,sizeof(struct sockaddr_in6)) != 0){
            fprintf(stderr, "Error in connect() \n");
            return -1;
        }
    }

    return sock;
}