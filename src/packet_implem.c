#include "packet_interface.h"

/**
 * Structure du packet telle que définie dans les spécifications
 **/

struct __attribute__((__packed__)) pkt {
    uint8_t window : 5;
    uint8_t tr : 1;
    uint8_t type : 2;
    uint8_t seqnum : 8;
    uint16_t length : 16;
    uint32_t timestamp : 32;
    uint32_t crc1 : 32;
    char* payload;
    uint32_t crc2 : 32;
};

/**
 * Fonctions de la librairie
 **/

pkt_t* pkt_new()
{
    pkt_t* pkt = (pkt_t*) calloc(1, sizeof(pkt_t));
    return pkt;
}

void pkt_del(pkt_t *pkt)
{
    if(pkt != NULL){
        if(pkt->length > 0 || pkt->payload != NULL){
            free(pkt->payload);
        }
        free(pkt);
    }
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
    /* Your code will be inserted here */
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
    /* Your code will be inserted here */
}

ptypes_t pkt_get_type  (const pkt_t* pkt)
{
    return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t* pkt)
{
    return pkt->tr;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
    return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
    return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt)
{
    return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt)
{
    return pkt->crc1;
}

uint32_t pkt_get_crc2   (const pkt_t* pkt)
{
    return pkt->crc2;
}

const char* pkt_get_payload(const pkt_t* pkt)
{
    return pkt->payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
    if(type < 1 || type > 3){
        return E_TYPE;
    }
    else{
        pkt->type = type;
        return PKT_OK;
    }
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
    if(tr != 0 && tr != 1){
        return E_TR;
    }
    else{
        pkt->tr = tr;
        return PKT_OK;
    }
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
    if(window > MAX_WINDOW_SIZE){
        return E_WINDOW;
    }
    else{
        pkt->window = window;
        return PKT_OK;
    }
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
    pkt->seqnum = seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
    if(length > MAX_PAYLOAD_SIZE){
        return E_LENGTH;
    }
    else{
        pkt->length = length;
        return PKT_OK;
    }
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
    pkt->timestamp = timestamp;
    return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
    pkt->crc1 = crc1;
    return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
    pkt->crc2 = crc2;
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length)
{
    if(length > 512 || length < 0){
        return E_NOMEM;
    }
    else{
        pkt->payload = (char*) malloc(length);
        if(pkt->payload == NULL){
            return E_NOMEM;
        }
        else{
            memcpy(pkt->payload, data, length);
            pkt_set_length(pkt, length);
            return PKT_OK;
        }
    }
}

ssize_t predict_header_length(const pkt_t *pkt)
{
    if(pkt_get_length(pkt) < 0 || pkt_get_length(pkt) > 512){
        return -1;
    }
    else{
        if(pkt_get_type(pkt) != PTYPE_DATA){
            return 0;
        }
        else{
            // TODO : Calculer la taille du header
        }
    }
}