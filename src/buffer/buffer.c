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
            int cpt = 0;
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

pkt_t* look_for_timedout_packet(buffer_t* buffer){
    pkt_t* pkt = NULL;
    if(!buffer){
        ERROR("Buffer is null");
        return pkt;
    }
    if(buffer->size == 0){
        //ERROR("No timed out packet buffer is empty");
        return pkt;
    }
    node_t* cur = buffer->first;
    uint32_t now_s;
    for(int i = 0 ; i < (int)buffer->size ; i++){
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

void buffer_print(buffer_t *buffer) {
    node_t* current = buffer->first;
    for(int i = 0 ; i < (int)buffer->size ; i++){
        fprintf(stderr, "%i -> ", pkt_get_seqnum(current->pkt));
        current = current->next;
    }
    fprintf(stderr,"END %ld\n",buffer->size);
}

// int main(){
//     buffer_t* buffer = buffer_init();

//     // for(int i = 3; i < 10; i++){
//     //     pkt_t* packet = pkt_new();
//     //     pkt_set_seqnum(packet, i);
//     //     buffer_enqueue(buffer, packet);
//     // }

//     /*** Packet 1 ***/
//     // pkt_t* p1 = pkt_new();
//     // pkt_set_seqnum(p1, 127);
//     // buffer_enqueue(buffer, p1);

//     // /*** Packet 2 ***/
//     // pkt_t* p2 = pkt_new();
//     // pkt_set_seqnum(p2, 128);
//     // buffer_enqueue(buffer, p2);

//     // /*** Packet 0 ***/
//     // pkt_t* p3 = pkt_new();
//     // pkt_set_seqnum(p3, 129);
//     // buffer_enqueue(buffer, p3);

//     // /*** Packet 11 ***/
//     // pkt_t* p4 = pkt_new();
//     // pkt_set_seqnum(p4, 130);
//     // buffer_enqueue(buffer, p4);

//     // buffer_print(buffer);

//     // /*** Packet 10 ***/
//     // pkt_t* packet10 = pkt_new();
//     // pkt_set_seqnum(packet10, 126);
//     // buffer_enqueue(buffer, packet10);

//     // buffer_print(buffer);

//     pkt_t* p4 = pkt_new();
//     pkt_set_seqnum(p4, 4);
//     buffer_enqueue(buffer, p4);

//     pkt_t* p6 = pkt_new();
//     pkt_set_seqnum(p6, 6);
//     buffer_enqueue(buffer, p6);



//     /*** Packet 12 ***/
//     pkt_t* packet12 = pkt_new();
//     pkt_set_seqnum(packet12, 255);
//     buffer_enqueue(buffer, packet12);

//     pkt_t* packet13 = pkt_new();
//     pkt_set_seqnum(packet13, 251);
//     buffer_enqueue(buffer, packet13);

//     pkt_t* p5 = pkt_new();
//     pkt_set_seqnum(p5, 5);
//     buffer_enqueue(buffer, p5);


//     pkt_t* packet14 = pkt_new();
//     pkt_set_seqnum(packet14, 252);
//     buffer_enqueue(buffer, packet14);

//     buffer_print(buffer);

//     pkt_t* packet15 = pkt_new();
//     pkt_set_seqnum(packet15, 3);
//     buffer_enqueue(buffer, packet15);


//     buffer_print(buffer);
// }
