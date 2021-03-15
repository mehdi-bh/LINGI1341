#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "packet/packet.h"
#include "socket/socket_manager.h"
#include "buffer/buffer.h"
#include "logs/log.h"

int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-s stats_filename] listen_ip listen_port", prog_name);
    return EXIT_FAILURE;
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

    int cpt = 0;
    pkt_t* pkt = pkt_new();
    ssize_t received;
    char msg[MAX_PKT_SIZE];
    // while(1){
    //     received = recv(sock, msg, MAX_PKT_SIZE, 0);
    //     pkt_decode(msg,received,pkt);
    //     printf("%s\n %ld\n",pkt_get_payload(pkt), received);
    //     cpt++;
    // }
    read_write_loop_receiver(sock,STDOUT_FILENO);

    return EXIT_SUCCESS;
}
