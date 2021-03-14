
#include "packet/packet.h"
#include "logs/log.h"
#include "socket/socket_manager.h"

#define BUFF_LEN 

int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-f filename] [-s stats_filename] receiver_ip receiver_port", prog_name);
    return EXIT_FAILURE;
}


// gcc sender.c -o sender
// ./sender ipv6 port
int main(int argc, char **argv) {
    int opt;

    char *filename = NULL;
    char *stats_filename = NULL;
    char *receiver_ip = NULL;
    char *receiver_port_err;
    uint16_t receiver_port;

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

    receiver_ip = argv[optind];
    receiver_port = (uint16_t) strtol(argv[optind + 1], &receiver_port_err, 10);
    if (*receiver_port_err != '\0') {
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

    ASSERT(1 == 1); // Try to change it to see what happens when it fails
    DEBUG_DUMP("Some bytes", 12); // You can use it with any pointer type
    DEBUG("DEBUG TEST");
    // This is not an error per-se.
    ERROR("Sender has following arguments: filename is %s, stats_filename is %s, receiver_ip is %s, receiver_port is %u",
        filename, stats_filename, receiver_ip, receiver_port);

    DEBUG("You can only see me if %s", "you built me using `make debug`");
    // Now let's code!

    // REGISTER 
    struct sockaddr_in6 receiver_addr;

    const char* err = real_address(receiver_ip,&receiver_addr);
    if(err){
        ERROR("Could not resolve hostname %s : %d",receiver_ip,receiver_port);
        exit(EXIT_FAILURE);
    }

    int sock = create_socket(NULL,-1,&receiver_addr,receiver_port);
    if(sock < 0){
        ERROR("Failed to create socket at %s : %d",receiver_ip,receiver_port);
        exit(EXIT_FAILURE);
    }

    char msg[32] = "hello, world!";
    pkt_t* pkt = pkt_new();
    pkt_set_type(pkt,PTYPE_DATA);
    pkt_set_payload(pkt,msg,sizeof(msg));
    pkt_set_tr(pkt,0);
    pkt_set_seqnum(pkt,0);
    pkt_set_window(pkt,31);
    pkt_set_timestamp(pkt,10);

    char buf[MAX_PKT_SIZE];
    ssize_t len;
    pkt_print(pkt);

    for(int i = 0; i < 5 ; i++){
        strcat(msg," work ");
        pkt_set_payload(pkt,msg,sizeof(msg));
        pkt_status_code st = pkt_encode(pkt,buf,&len);
        printf("Error fdp : %i\n",st);

        send(sock, buf,len, 0);
    }
    
    return EXIT_SUCCESS;
}
