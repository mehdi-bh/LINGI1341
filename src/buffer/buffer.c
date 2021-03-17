#include "buffer.h"



buffer_t *buffer_init() {
    return (buffer_t *) calloc(1, sizeof(buffer_t));
}

pkt_t* look_for_timedout_packet(buffer_t* buffer){
    pkt_t* pkt = NULL;
    if(!buffer){
        ERROR("Buffer is null");
        return pkt;
    }
    if(buffer->size == 0){
        ERROR("No timed out packet buffer is empty");
        return pkt;
    }
    node_t* cur = buffer->first;
    time_t now;
    uint32_t now_s;
    for(int i = 0 ; i < buffer->size ; i++){
        now_s = (uint32_t) time(NULL);
        if(now_s - pkt_get_timestamp(cur->pkt) > RTO ){
            ERROR("Packet %d timed out",pkt_get_seqnum(cur->pkt));
            pkt_set_timestamp(cur->pkt,now_s);
            return cur->pkt;
        }
        cur = cur->next;
    }
        
    return NULL;
}


int buffer_enqueue(buffer_t *buffer, pkt_t *pkt) {
    node_t *newNode = (node_t *) malloc(sizeof(node_t));
    if(newNode == NULL) {
        fprintf(stderr, "Out of memory at node creation\n");
        return -1;
    }
    newNode->pkt = pkt;
    if(buffer->size == 0) {
        newNode->next = newNode;
        newNode->prev = newNode;
        buffer->first = newNode;
        buffer->last = newNode;
    } else {
        int first = 0;
        node_t* current = buffer->last;
        uint8_t pkt_seqnum = pkt_get_seqnum(pkt);
        while(pkt_seqnum < pkt_get_seqnum(current->pkt) && !(pkt_get_seqnum(current->pkt) > 200 && pkt_seqnum < 100)) {
            current = current->prev;
            if(current == buffer->first) {
                if(pkt_seqnum < pkt_get_seqnum(buffer->first->pkt)){
                    first = 1;
                }
                break;
            }
        }
        if(first){
            newNode->prev = current->prev;
            newNode->next = current;
            current->prev = newNode;
            buffer->first = newNode;
        }
        else{
            newNode->next = current->next;
            newNode->prev = current;
            current->next->prev = newNode;
            current->next = newNode;
            if(current == buffer->last) {
                buffer->last = newNode;
            }
        }
    }
    buffer->size += 1;

    return 0;
}

pkt_t *buffer_remove(buffer_t *buffer, uint8_t seqnum) {
    node_t *current = buffer->first;
    if(current == NULL) {
        return NULL;
    }

    while(pkt_get_seqnum(current->pkt) != seqnum) {
        current = current->next;
        if(current == buffer->first) {
            fprintf(stderr, "Node to remove (%i) not in buffer.\n", seqnum);
            return NULL;
        }
    }

    if(current == buffer->first) {
        if(buffer->size <= 1) {
            buffer->first = NULL;
            buffer->last = NULL;
            pkt_t *packet = current->pkt;
            node_free(current);
            buffer->size = 0;
            return packet;
        }
        current->prev->next = current->next;
        current->next->prev = current->prev;
        buffer->first = buffer->first->next;

    } else if(current == buffer->last) {
        current->prev->next = current->next;
        current->next->prev = current->prev;
        buffer->last = buffer->last->prev;

    } else {
        current->prev->next = current->next;
        current->next->prev = current->prev;
    }
    buffer->size -= 1;

    if(buffer->size == 0) {
        buffer->first = NULL;
        buffer->last = NULL;
    }

    current->next = NULL;
    current->prev = NULL;

    pkt_t *toReturn = current->pkt;
    node_free(current);

    return toReturn;
}

int buffer_remove_acked(buffer_t *buffer, uint8_t seqnum) {
    if(buffer->size == 0) {
        return 0;
    }

    int count = 0;

    while(seqnum != pkt_get_seqnum(buffer->first->pkt)) {
        pkt_del(buffer_remove(buffer, pkt_get_seqnum(buffer->first->pkt)));
        count++;
        if(buffer->size == 0) {
            break;
        }
    }

    return count;
}

pkt_t *buffer_get_pkt(buffer_t *buffer, uint8_t seqnum) {
    if(buffer->size == 0) {
        return NULL;
    }

    node_t *current = buffer->first;
    while(pkt_get_seqnum(current->pkt) != seqnum) {
        current = current->next;
        if(current == buffer->first) {
            return NULL;
        }
    }
    return current->pkt;
}

size_t buffer_size(buffer_t *buffer) {
    return buffer->size;
}

void buffer_free(buffer_t *buffer) {
    while(buffer->first != NULL) {
        pkt_del(buffer_remove(buffer, pkt_get_seqnum(buffer->first->pkt)));
    }
    if(buffer_size(buffer) != 0) {
        fprintf(stderr, "Exiting buffer_free() with buffer_size != 0\n");
    }

    free(buffer);
}

void node_free(node_t *node){
    if(node == NULL) {
        return;
    }
    free(node);
    node = NULL;
}

int is_in_buffer(buffer_t *buffer, uint8_t seqnum) {
    if (buffer == NULL || buffer->size == 0) {
        return 0;
    }

    node_t *current = buffer->first;
    for (int i = 0; i < (int)buffer_size(buffer); i++) {
        if (pkt_get_seqnum(current->pkt) == seqnum) {
            return 1;
        }
        current = current->next;
    }
    return 0;

}

void buffer_print(buffer_t *buffer, int amount) {
    node_t* current = buffer->first;
    for(int i = 0 ; i < (int)buffer->size ; i++){
        fprintf(stderr, "%i -> ", pkt_get_seqnum(current->pkt));
        current = current->next;
    }
    fprintf(stderr,"END %d\n",amount);
}

int main(){
    buffer_t* buffer = buffer_init();

    for(int i = 3; i < 10; i++){
        pkt_t* packet = pkt_new();
        pkt_set_seqnum(packet, i);
        buffer_enqueue(buffer, packet);
    }

    /*** Packet 1 ***/
    pkt_t* packet1 = pkt_new();
    pkt_set_seqnum(packet1, 1);
    buffer_enqueue(buffer, packet1);

    /*** Packet 2 ***/
    pkt_t* packet2 = pkt_new();
    pkt_set_seqnum(packet2, 2);
    buffer_enqueue(buffer, packet2);

    /*** Packet 0 ***/
    pkt_t* packet0 = pkt_new();
    pkt_set_seqnum(packet0, 0);
    buffer_enqueue(buffer, packet0);

    /*** Packet 11 ***/
    pkt_t* packet11 = pkt_new();
    pkt_set_seqnum(packet11, 11);
    buffer_enqueue(buffer, packet11);

    /*** Packet 10 ***/
    pkt_t* packet10 = pkt_new();
    pkt_set_seqnum(packet10, 10);
    buffer_enqueue(buffer, packet10);

    /*** Packet 12 ***/
    pkt_t* packet12 = pkt_new();
    pkt_set_seqnum(packet12, 12);
    buffer_enqueue(buffer, packet12);

    buffer_print(buffer,0);
}