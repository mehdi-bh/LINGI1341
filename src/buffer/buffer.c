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
    //newNode->pkt->seqnum = pkt_get_seqnum(pkt);
    if(buffer->size == 0) {
        newNode->next = newNode;
        newNode->prev = newNode;
        buffer->first = newNode;
        buffer->last = newNode;
    } else {
        node_t* current = buffer->last;
        uint8_t pkt_seqnum = pkt_get_seqnum(pkt);
        while(pkt_seqnum < pkt_get_seqnum(current->pkt) && !(pkt_get_seqnum(current->pkt) > 200 && pkt_seqnum < 100)) { // condition for separating two blocks of 256
            current = current->prev;
            if(current == buffer->first) {
                break;
            }
        }
        // insert in front of current
        newNode->next = current->next;
        newNode->prev = current;
        current->next->prev = newNode;
        current->next = newNode;
        if(current == buffer->last) {
            buffer->last = newNode;
        } else if(newNode->next == buffer->first) {
            buffer->first = newNode;
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
    // int amount2;
    // if (amount == 0) {
    //     amount2 = buffer_size(buffer);
    // } else {
    //     amount2 = amount;
    // }
    // node_t *current = buffer->first;
    // fprintf(stderr, "buffer CONTENT : ");
    // for (int i=1; i <= (int)buffer_size(buffer) && i <= amount2; i++) {
    //     fprintf(stderr, "%i -> ", pkt_get_seqnum(current->pkt));
    //     current = current->next;
    // }
    // if (amount != 0) {
    //     fprintf(stderr, "...\n");

    // } else {
    //     fprintf(stderr, "END\n");
    // }
    
    node_t* current = buffer->first;
    for(int i = 0 ; i < (int)buffer->size ; i++){
        fprintf(stderr, "%i -> ", pkt_get_seqnum(current->pkt));
        current = current->next;
    }
    fprintf(stderr,"END %d\n",amount);
}

// int main(int argc, char* argv[]){
//     buffer_t* buffer = buffer_init();
//     for(int i = 0; i < 257; i++){
//         pkt_t* pkt = pkt_new();
//         char* a = "SalutSalutArbitre";

//         pkt_set_payload(pkt, a, 17);
//         pkt_set_type(pkt,PTYPE_DATA);
//         pkt_set_tr(pkt,0);
//         pkt_set_window(pkt,10);
//         pkt_set_seqnum(pkt,i);
//         pkt_set_timestamp(pkt,11);
//         size_t length = (4*sizeof(uint32_t) + pkt_get_length(pkt));
//         char* buf = (char*)malloc(sizeof(char)*100);

//         pkt_encode(pkt, buf, &length);
//         buffer_enqueue(buffer,pkt);
//     }

//     buffer_print(buffer,0);
// }