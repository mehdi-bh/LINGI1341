#include "read_write_loop.h"
#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
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
