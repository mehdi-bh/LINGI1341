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

buffer_t* buffer_init();

pkt_t* look_for_timedout_packet(buffer_t* buffer);

pkt_t* look_for_unsended_packet(buffer_t* buffer);

pkt_t* buffer_remove_first(buffer_t* buffer);

int buffer_enqueue(buffer_t *buffer, pkt_t *pkt);

pkt_t* buffer_remove(buffer_t* buffer, uint8_t seqnum);

int buffer_remove_acked(buffer_t* buffer, uint8_t seqnum);

pkt_t* buffer_get_pkt(buffer_t* buffer, uint8_t seqnum);

size_t buffer_size(buffer_t* buffer);

void buffer_print(buffer_t* buffer);

int is_in_buffer(buffer_t* buffer, uint8_t seqnum);

void buffer_free(buffer_t* buffer);

void node_free(node_t* node);

#endif