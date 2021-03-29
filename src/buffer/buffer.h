#ifndef BUFFER_H
#define BUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "../logs/log.h"
#include "../packet/packet.h"

#define RTO 2

typedef struct node {
    struct node *next;
    struct node *prev;
    struct pkt *pkt;
} node_t;

typedef struct buffer {
    struct node *first;
    struct node *last;
    size_t size;
} buffer_t;

/*
 * Function:  buffer_init 
 * ----------------------
 * Creates a new buffer_t and returns it. 
 *
 * @return: (buffer_t) A pointer to the buffer allocated with calloc()
 */
buffer_t* buffer_init();

/*
 * Function:  buffer_enqueue
 * -------------------------
 * Adds a packet in the buffer at the right place,
 * which means that it respect the order of the seqnum.
 * Imagine the receiver side with a packet missing in the center of the
 * receiving window, this function will put it at the right place.
 *
 * @buffer: The buffer in which we want to add a packet.
 * @return: 0 if the packet has been added with success.
 *          1 if an error occurred.
 */
int buffer_enqueue(buffer_t *buffer, pkt_t *pkt);

/*
 * Function:  buffer_remove_first
 * ------------------------------
 * Dequeues the first packet of the buffer and returns it.
 *
 * @buffer: The buffer in which we want to remove the first packet.
 * @return: (pkt_t) if the first packet of the buffer has been removed with success.
 *          (NULL)  if the buffer is empty or an error occurred.
 */
pkt_t* buffer_remove_first(buffer_t* buffer);

/*
 * Function:  buffer_remove
 * ------------------------
 * Removes a packet in the buffer according to a seqnum
 * given as parameter and returns the packet.
 *
 * @buffer: The buffer in which we want to remove a packet.
 * @seqnum: The seqnum of the packet we want to remove.
 * @return: (pkt_t) if the packet has been found and removed with success.
 *          (NULL)  if the packet has not been found or an error occurred.
 */
pkt_t* buffer_remove(buffer_t* buffer, uint8_t seqnum);

/*
 * Function:  buffer_remove_acked
 * ------------------------------
 * Removes all the packet that have been acked according cumulative ack.
 * It removes all the packet with seqnum lower than the one passed as parameter.
 *
 * @buffer: The buffer in which we want to remove the acked packets.
 * @seqnum: The seqnum of the ack we have received.
 * @return: (int) the number of packets removed.
 *          0     if no packet has been removed.
 */
int buffer_remove_acked(buffer_t* buffer, uint8_t seqnum);

/*
 * Function:  buffer_get_pkt
 * -------------------------
 * Returns a packet found on his seqnum. Useful when the receiver has (not
 * received)/(received truncated) a packet and the sender has to resend it for example.
 *
 * @buffer: The buffer in which we want to find a packet.
 * @seqnum: The seqnum of the packet we want to find.
 * @return: (pkt_t) if found, the packet.
 *          (NULL)  if not found or an error occurred.
 */
pkt_t* buffer_get_pkt(buffer_t* buffer, uint8_t seqnum);

/*
 * Function:  look_for_timedout_packet
 * -----------------------------------
 * Scans the buffer looking for packets that have timed out. 
 * By computing (now - timestamp(pkt) > RTO)
 * which means that the packet is expired
 * and returns it.
 *
 * @buffer: The buffer in which we want to find a timed out packet
 * @return: (pkt_t) if found, a timed out packet, the oldest.
 *          (NULL)  if no timed out packet has been found or an error occured.
 */
pkt_t* look_for_timedout_packet(buffer_t* buffer);

/*
 * Function:  look_for_unsent_packet
 * ---------------------------------
 * Scans the buffer looking for packets that have not been sent.
 * By watching (timestamp == 0)
 * which means that the packet has not been sent
 * and returns it.
 *
 * @buffer: The buffer in which we want to find an unsent packet
 * @return: (pkt_t) if found, the first packet in the buffer that has not been sent
 *          in seqnum increasing order.
 *          (NULL)  if no unsent packet found or an error occurred.
 */
pkt_t* look_for_unsended_packet(buffer_t* buffer);

/*
 * Function:  is_in_buffer
 * -----------------------
 * Checks if a seqnum is in the buffer.
 *
 * @buffer: The buffer in which we want to check if a seqnum is present.
 * @seqnum: The seqnum we want to check.
 * @return: 1 if found.
 *          0 if not found or the buffer is empty.
 */
int is_in_buffer(buffer_t* buffer, uint8_t seqnum);

/*
 * Function:  buffer_size
 * ----------------------
 * Returns the size of a buffer.
 *
 * @buffer: The buffer of which we want to get the size.
 * @return: (size_t) the size of the buffer.
 */
size_t buffer_size(buffer_t* buffer);

/*
 * Function:  buffer_free
 * ----------------------
 * Deletes all the packets remaining in the buffer and frees the memory.
 *
 * @buffer: The buffer we want to free.
 * @return: void.
 */
void buffer_free(buffer_t* buffer);

/*
 * Function:  node_free
 * --------------------
 * Frees a node.
 *
 * @node: The node we want to free.
 * @return: void.
 */
void node_free(node_t* node);

/*
 * Function:  buffer_print
 * -----------------------
 * Prints with fprintf the buffer passed as parameter.
 * The output is formated as a linked list to easily see the seqnum of
 * the packets that we have in the buffer.
 * example : 
 * 1 -> 2 -> 3 -> 4 -> END
 *
 * @buffer: The buffer we want to print.
 * @return: void.
 */
void buffer_print(buffer_t* buffer);

#endif