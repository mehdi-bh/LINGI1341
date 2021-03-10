#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include "wait_for_client.h"

int wait_for_client(int sfd){
    char buffer[1024];
    
    struct sockaddr_in6 sock;
    socklen_t len = sizeof(sock);

    int nread = recvfrom(sfd, buffer, sizeof(char)*1024, MSG_PEEK, (struct sockaddr*) &sock, &len);
    if(nread == -1){
        fprintf(stderr, "Error using recvfrom\n");
        return -1;
    }

    int done = connect(sfd, (struct sockaddr*) &sock, (int) len);
    if(done == -1){
        fprintf(stderr, "Error using connect");
        return -1;
    }

    return 0;
}