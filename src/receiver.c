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
#include <time.h>

#include "packet/packet.h"
#include "socket/socket_manager.h"
#include "buffer/buffer.h"
#include "logs/log.h"

#define MAX_PKT_SIZE 512 + 16
#define MAX_SEQNUM 256
#define ACK_SIZE 10
#define MAX_WINDOW 31

int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-s stats_filename] listen_ip listen_port", prog_name);
    return EXIT_FAILURE;
}

int window = MAX_WINDOW;
int last_acked = 0;
int last_writed = -1;
int nb_writed = 0;
int potentialEOF = 0;
int lastack_to_send = -2;

int fd = STDOUT_FILENO;

int stats_data_received = 0;
int stats_data_truncated_received = 0;
int stats_ack_sent = 0;
int stats_nack_sent = 0;
int stats_packet_ignored = 0;
int stats_packet_duplicated = 0;


/*
 * Function:  is_in_window
 * -------------------------
 * Checks that the seqnum of a packet received 
 * is in the receiving window.
 *
 * @seqnum: (int) The seqnum of the packet we want to check.
 * @return: 1 The seqnum is in window.
 *          0 The seqnum is not in window.
 */
int is_in_window(int seqnum){
    if(seqnum == last_writed) return 0;
    if(seqnum < last_writed){
        if(seqnum + MAX_SEQNUM - last_writed <= window)
            return 1;
        return 0;
    }else{
        if(seqnum - last_writed < window)
            return 1;
        return 0;
    }
}

/*
 * Function:  write_buffer_to_file
 * -------------------------------
 * The receiving buffer is written in the file during the execution of the 
 * receiver. When the receiver receives packets without problem, he writes them
 * directly in the file, when the receiver receives some packets but a packet is missing,
 * he will wait to receive this packet and then write all the packets that are in order and
 * completes. 
 *
 * @buffer: The buffer we want to write the packets from.
 * @fdOut:  The file in which we want to write.
 * @return: 1 if everything worked successfully.
 *         -1 if an error occurred.
 */
int write_buffer_to_file(buffer_t* buffer, const int fdOut){
    node_t* current = buffer->first;
    pkt_t* pkt;
    buffer_print(buffer);
    while(current != NULL && pkt_get_seqnum(current->pkt) == (last_writed+1)%MAX_SEQNUM ){
        pkt = current->pkt;
        ssize_t writed = write(fdOut,pkt_get_payload(pkt),pkt_get_length(pkt));
        if(writed == -1){
            ERROR("Receiver can't write to file");
            return -1;
        }
        ERROR("packet %d writed",pkt_get_seqnum(pkt));
        last_writed = pkt_get_seqnum(pkt);

        current = current->next;
        pkt = buffer_remove(buffer,pkt_get_seqnum(pkt));  
        nb_writed++;
    }
    return 1;
}

/*
 * Function:  send_ack
 * -------------------
 * This function sends an acknowledgement to the sender, it holds
 * the ACK and the NACK types.
 *
 * @sfd: Socket file descriptor on which the receiver can send.
 * @seqnum:  Seqnum of the acknowledgement
 * @type: ACK or NACK
 * @wind: Size of the window, which means the remaining space
 * in buffer considering the window.
 * @return: 0 if everything worked successfully.
 *         -1 if an error occurred.
 */
int send_ack(const int sfd,int seqnum, uint8_t type,int wind){
    pkt_t* ack = pkt_new();
    if(!ack){
        ERROR("Can't create ack packet");
        return -1;
    }
    pkt_set_type(ack,type);
    pkt_set_seqnum(ack,seqnum);
    pkt_set_tr(ack,0);
    pkt_set_window(ack,wind);
    pkt_set_timestamp(ack,(uint32_t) time(NULL));
    pkt_set_payload(ack,NULL,0);

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
    if(type == PTYPE_ACK){
        last_acked = seqnum;
        ERROR("ack %d sended",seqnum);
        stats_ack_sent++;
    }
    else{
        ERROR("nack %d sended",seqnum);
        stats_nack_sent++;
    }


    return 0;
}

/*
 * Function:  read_write_loop_receiver
 * -----------------------------------
 * This function is the most important one for the receiver.
 * It reads the packets from the sender on the socket and sends acknowledgements while
 * writing into the output file. All that implementing the selective repeat and sending
 * cumulative acknowledgements to not overload the network. It also recognizes truncated
 * packets and asks a resend from the sender for these cases.
 *
 * @sfd: Socket file descriptor to communicate with the sender.
 * @fdOut: File descriptor of the output file received.
 * @return: void
 */
void read_write_loop_receiver(const int sfd,const int fdOut){

    int nfds = 1;
    struct pollfd *pfds;
    pfds = malloc(sizeof(struct pollfd)*nfds);
    buffer_t* buffer = buffer_init();
    int ready =-1;
    struct timespec start,end;
    int reset_time = 1;
    int n_accumulated = 0;
    int bypass = 0;
    //srand(time(NULL));

    while(last_acked != lastack_to_send 
        && ready != 0
        //&& potentialEOF != 2
    ){

        pfds[0].fd = sfd;
        pfds[0].events = POLLIN;

        
        ready = poll(pfds,nfds,10 * 1000);
        if(ready == -1){
            ERROR("Poll error");
            return;
        }
        if(ready == 0){
            ERROR("Connection timed out, consider finished");
        }

        int error;
        ssize_t s;
        pkt_t* pkt = pkt_new();
        pkt_status_code st;
        char buf[MAX_PKT_SIZE];
        if(pfds[0].revents != 0 && pfds[0].revents & POLLIN){
            s = read(pfds[0].fd,buf,MAX_PKT_SIZE);
            ERROR("%ld bytes readed",s);
            if(s == -1){
                ERROR("Can't read socket");
            }

            if(reset_time){
                clock_gettime(CLOCK_MONOTONIC_RAW,&start);
                reset_time = 0;
            }
            clock_gettime(CLOCK_MONOTONIC_RAW,&end);
            n_accumulated++;

            st = pkt_decode(buf,s,pkt);
            if(st != PKT_OK || pkt_get_type(pkt) != PTYPE_DATA){
                ERROR("Packet incorrect : %d",st);pkt_del(pkt);bypass=1;
                stats_packet_ignored++;
            }else{
                ERROR("Packet [%d] received, nb writed [%d], window [%d]",pkt_get_seqnum(pkt),nb_writed,window);
                stats_data_received += 1;

                if(is_in_buffer(buffer,pkt_get_seqnum(pkt))){ 
                    ERROR("Duplicate packet [%d]",pkt_get_seqnum(pkt));bypass=1;
                    stats_data_truncated_received++;
                }
                else if(is_in_window(pkt_get_seqnum(pkt))){
                    
                    if(pkt_get_tr(pkt) == 1){
                        send_ack(sfd,pkt_get_seqnum(pkt), PTYPE_NACK, window - buffer->size);
                        continue;
                    }
                    else if(pkt_get_type(pkt) == PTYPE_DATA && pkt_get_length(pkt) == 0){
                        if(pkt_get_seqnum(pkt) == (nb_writed) % MAX_SEQNUM){
                            ERROR("EOF for sender");
                            lastack_to_send = pkt_get_seqnum(pkt);
                            bypass=1;
                        }
                    }
                    else{
                        error = buffer_enqueue(buffer,pkt);
                        if(error == -1){
                            ERROR("Out of memory while adding to buffer");return;
                        }
                        
                        error = write_buffer_to_file(buffer,fdOut);
                        if(error == -1){
                            ERROR("Error while writing");
                            //continue;
                        }
                        if(window > MAX_WINDOW) window = MAX_WINDOW;
                    }
                }
                else{
                    ERROR("Packet not in window [%d]",pkt_get_seqnum(pkt));
                }
            }
            if( (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_nsec - start.tv_nsec) / 1000.0f > 500.0f
                || n_accumulated >= MAX_WINDOW/4 
                || bypass
                || nb_writed == 1){
                send_ack(sfd,(last_writed + 1) % MAX_SEQNUM, PTYPE_ACK,window - buffer->size);
                n_accumulated = 0;
                reset_time = 1;
                bypass = 0;
            }
        }
    }
}

/*
 * Function:  write_stats_in_file
 * ------------------------------
 * This functions writes all the stats of the file transfer into
 * a specified file.
 *
 * @filename: Name of the file in which we write the stats
 * @return: 0 if everything worked successfully.
 *         -1 if an error occurred.
 */
int write_stats_in_file(char* filename){
    FILE *file = fopen(filename,"w");
    if(file == NULL){
        return -1;
    }

    fprintf(file,"data_received:%d\n",stats_data_received);
    fprintf(file,"data_truncated_received:%d\n",stats_data_truncated_received);
    fprintf(file,"ack_sent:%d\n",stats_ack_sent);
    fprintf(file,"nack_sent:%d\n",stats_nack_sent);
    fprintf(file,"packet_ignored:%d\n",stats_packet_ignored);
    fprintf(file,"packet_duplicated:%d\n",stats_packet_duplicated);
    return 0;
}

int main(int argc, char **argv) {
    int opt;

    char *filename = NULL;
    char *stats_filename = NULL;
    char *listen_ip = NULL;
    char *listen_port_err;
    uint16_t listen_port;

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

    listen_ip = argv[optind];
    listen_port = (uint16_t) strtol(argv[optind + 1], &listen_port_err, 10);
    if (*listen_port_err != '\0') {
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

    ERROR("Receiver has following arguments: stats_filename is %s, listen_ip is %s, listen_port is %u",
        stats_filename, listen_ip, listen_port);

    struct sockaddr_in6 listener_addr;

    if(filename){
        fd = open(filename,O_WRONLY | O_RDONLY | O_CREAT,0644);
        if(fd < 0){
            ERROR("Can't open file %s",filename);
        }
    }

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

    read_write_loop_receiver(sock,fd);

    int stats_written = write_stats_in_file(stats_filename);
    if(stats_written != 0){
        ERROR("Stats writing failed");
    }

    return EXIT_SUCCESS;
}