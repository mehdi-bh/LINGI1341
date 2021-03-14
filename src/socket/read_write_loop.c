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

    // fd_set reader;

    // int c = 1;
    // int retval;

    // char buffer[1024];

    // while(1){
    //     FD_ZERO(&reader);
    //     FD_SET(sfd, &reader);
    //     FD_SET(STDIN_FILENO, &reader);

    //     retval = select(sfd+1, &reader, NULL, NULL, 0);
    //     if(retval == -1){
    //         fprintf(stderr, "Error select\n");
    //     }

    //     else if(FD_ISSET(STDIN_FILENO, &reader)){
    //         c = read(STDIN_FILENO, buffer, sizeof(buffer));

    //         if(c < 0){
    //             fprintf(stderr, "Error writing 2\n");
    //             return;
    //         }

    //         size_t readed = (size_t) c;

    //         int error = send(sfd, buffer, readed, 0);
    //         if(error == -1){
    //             fprintf(stderr, "Error while writing in sfd\n");
    //         }
    //     }
    //     else if(FD_ISSET(sfd, &reader)){
    //         c = read(sfd, buffer, sizeof(buffer));
    //         if(c < 0){
    //             fprintf(stderr, "Error reading\n");
    //             return;
    //         }

    //         size_t readed = (size_t) c;

    //         int error = write(STDOUT_FILENO, buffer, readed);

    //         if(error == -1){
    //             fprintf(stderr, "Error while writing in stdout\n");
    //             return;
    //         }
    //     }
    // }

}
