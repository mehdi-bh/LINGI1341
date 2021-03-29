#include "buffer.h"


buffer_t *buffer_init() {
    return (buffer_t *) calloc(1, sizeof(buffer_t));
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
        if(pkt_get_seqnum(current->pkt) < 50 && pkt_seqnum > 205){
            current = buffer->first;
            if(pkt_seqnum < pkt_get_seqnum(current->pkt)
                || (pkt_seqnum > pkt_get_seqnum(current->pkt) && pkt_get_seqnum(current->pkt) < 50)){
                first = 1;
            }
            while(pkt_seqnum > pkt_get_seqnum(current->next->pkt)  && first != 1
                && !(pkt_get_seqnum(current->next->pkt) < 50  && pkt_seqnum > 205)){
                current = current->next;
            }

        }
        else{
            while(pkt_seqnum < pkt_get_seqnum(current->pkt) 
                && !(pkt_get_seqnum(current->pkt) > 205  && pkt_seqnum < 50)){
                current = current->prev;
                if(current == buffer->first) {
                    if(pkt_seqnum < pkt_get_seqnum(buffer->first->pkt)){
                        first = 1;
                    }
                    break;
                }
            }
        }
        if(first){
            newNode->prev = current->prev;
            newNode->next = current;
            current->prev->next = newNode;
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

pkt_t* buffer_remove_first(buffer_t* buffer){
    pkt_t* pkt = NULL;
    if(!buffer){
        return pkt;
    }
    if(buffer->size == 0){
        return pkt;
    }
    pkt = buffer->first->pkt;
    node_t* current = buffer->first;
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
    pkt = current->pkt;
    node_free(current);
    buffer->size -= 1;
    return pkt;
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
    if(!is_in_buffer(buffer,seqnum)){
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

pkt_t* look_for_timedout_packet(buffer_t* buffer){
    pkt_t* pkt = NULL;
    if(!buffer){
        ERROR("Buffer is null");
        return pkt;
    }
    if(buffer->size == 0){
        return pkt;
    }
    node_t* cur = buffer->first;
    uint32_t now_s;
    int found = 0;
    for(int i = 0 ; i < (int)buffer->size ; i++){
        now_s = (uint32_t) time(NULL);
        if(pkt_get_timestamp(cur->pkt) != 0 &&
         now_s - pkt_get_timestamp(cur->pkt) > RTO ){
            if(!found){
                ERROR("Packet %d timed out",pkt_get_seqnum(cur->pkt));
                pkt_set_timestamp(cur->pkt,now_s);
                pkt = cur->pkt;
                found = 1;
            }
            else{
                pkt_set_timestamp(cur->pkt,now_s - RTO/2);
            }
        }
        cur = cur->next;
    }
    return pkt;
}

pkt_t* look_for_unsended_packet(buffer_t* buffer){
    pkt_t* pkt = NULL;
    if(!buffer){
        ERROR("Buffer is null");
        return pkt;
    }
    if(buffer->size == 0){
        return pkt;
    }
    node_t* cur = buffer->first;
    for(int i = 0 ; i < (int)buffer->size ; i++){
        if(pkt_get_timestamp(cur->pkt) == 0 ){
            return cur->pkt;
        }
        cur = cur->next;
    }
    return NULL;
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

void buffer_print(buffer_t *buffer) {
    node_t* current = buffer->first;
    for(int i = 0 ; i < (int)buffer->size ; i++){
        fprintf(stderr, "%i -> ", pkt_get_seqnum(current->pkt));
        current = current->next;
    }
    fprintf(stderr,"END %ld\n",buffer->size);
}