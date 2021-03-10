#include "create_socket.h"
#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
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

