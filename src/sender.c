#include <sys/stat.h>
#include <fcntl.h>

#include "packet/packet.h"
#include "logs/log.h"
#include "socket/socket_manager.h"
#include "buffer/buffer.h"

#define BUFF_LEN 
#define WINDOW 31

int fd = STDIN_FILENO;
int seqnum = 0;


int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-f filename] [-s stats_filename] receiver_ip receiver_port", prog_name);
    return EXIT_FAILURE;
}

pkt_t* read_file(buffer_t* buffer, int fd){
    if(!buffer){
        ERROR("Buffer is null");
        return NULL;
    }
    
    char paylaod[MAX_PAYLOAD_SIZE];
    ssize_t readed = read(fd,paylaod,MAX_PAYLOAD_SIZE);

    if(readed == 0){
        return NULL;
    }

    if(readed == -1){
        ERROR("Can't read data in file");
        return NULL;
    }
    pkt_t* pkt = pkt_new();
    if(!pkt){
        ERROR("Can't create packet");
        return NULL;
    }
    pkt_status_code err = pkt_set_type(pkt,PTYPE_DATA);
    err = pkt_set_tr(pkt,0);
    err = pkt_set_window(pkt,0);
    err = pkt_set_seqnum(pkt,(seqnum++ % 256));
    err = pkt_set_payload(pkt,paylaod,readed);

    if(err){
        ERROR("Can't set data to the packet : %d",err);
        pkt_del(pkt);
        return NULL;
    }

    return pkt;
}

// gcc sender.c packet/packet.c logs/log.c socket/socket_manager.c buffer/buffer.c -o sender -lz
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
    
    ERROR("Sender has following arguments: filename is %s, stats_filename is %s, receiver_ip is %s, receiver_port is %u",
        filename, stats_filename, receiver_ip, receiver_port);

    DEBUG("You can only see me if %s", "you built me using `make debug`");

    struct sockaddr_in6 receiver_addr;

    if(filename){
        fd = open(filename,O_RDWR);
        if(fd < 0){
            ERROR("Can't open file %s",filename);
        }
    }

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

    pkt_t* pkt;
    char buf[MAX_PKT_SIZE];
    ssize_t len;

    buffer_t* buffer = buffer_init();
    
    pkt_status_code st;

    while( (pkt = read_file(buffer,fd)) != NULL){
        //pkt_print(pkt);
        buffer_enqueue(buffer,pkt);
        st = pkt_encode(pkt,buf,&len);
        send(sock,buf,len,0);
    }

    return EXIT_SUCCESS;
}
