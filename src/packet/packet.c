#include "packet.h"

struct __attribute__((__packed__)) pkt {
    uint8_t type        : 2;
    uint8_t tr          : 1;
    uint8_t window      : 5;
    uint16_t length     ;
    uint8_t seqnum      ;
    uint32_t timestamp  ;
    uint32_t crc1       ;
    char* payload;
    uint32_t crc2       ;
}pkt;

pkt_t* pkt_new()
{
    pkt_t* pkt = calloc(0,sizeof(pkt_t));
    if(!pkt)
        return NULL;
    return pkt;
}

void pkt_del(pkt_t *pkt)
{
    if(pkt != NULL){
        if(pkt->length != 0 || pkt->payload != NULL){
            free(pkt->payload);
        }
        free(pkt);
    }    
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
    if(!data)
        return E_UNCONSISTENT;
    if( len > MAX_PAYLOAD_SIZE + 16 || len < 10)
        return E_LENGTH;
    

    memset(pkt,0,sizeof(pkt_t));

    pkt_status_code st;

    uint8_t head;
    int cur = 0;
    memcpy(&head,data + cur,sizeof(uint8_t));
    cur += 1;
    
    pkt->type = (head & 0xC0) >> 6;
    pkt->tr = (head & 0x20) >> 5;
    pkt->window = (head & 0x1F);

    if( (st = pkt_set_type(pkt,pkt->type)) != PKT_OK){
        fprintf(stderr,"Incorrect type\n");
        return st;
    }

    if( (st = pkt_set_tr(pkt,pkt->tr)) != PKT_OK){
        fprintf(stderr,"Truncated incorrect\n");
        return st;
    }

    if( (st = pkt_set_window(pkt,pkt->window)) != PKT_OK){
        fprintf(stderr,"Window invalid\n");
        return st;
    }

    if(pkt->type == PTYPE_DATA){
        memcpy(&(pkt->length),data + cur,sizeof(uint16_t));
        cur += 2;

        if( (st = pkt_set_length(pkt,ntohs(pkt->length))) != PKT_OK){
            fprintf(stderr,"Incorrect length\n");
            return st;
        }
    }

    memcpy(&(pkt->seqnum),data + cur, sizeof(uint8_t));
    cur += 1;

    if( (st = pkt_set_seqnum(pkt,pkt->seqnum)) != PKT_OK){
        fprintf(stderr,"Incorrect seqnum\n");
        return st;
    }

    memcpy(&(pkt->timestamp), data + cur, sizeof(uint32_t));
    cur += 4;

    if( (st = pkt_set_timestamp(pkt,pkt->timestamp)) != PKT_OK){
        fprintf(stderr,"Incorrect timestamp\n");
        return st;
    }

    uint32_t crc1;
    memcpy(&crc1,data + cur,sizeof(uint32_t));
    cur += 4;
    pkt->crc1 = ntohl(crc1);

    uint8_t tr = pkt->tr;
    pkt_set_tr(pkt,0);

    uint32_t crc1_test = crc32(crc32(0L,Z_NULL,0),(Bytef*) data,(uInt) sizeof(char)* (cur-4) ); //cur - 4 = nbr byte writed - crc length
    
    if(pkt->crc1 != crc1_test){
        fprintf(stderr,"Not equals crc1\n");
        return E_CRC;
    }

    if(pkt->type != PTYPE_DATA){
        pkt_set_tr(pkt,tr);
        return PKT_OK;
    }

    if( (st = pkt_set_payload(pkt,data + cur,pkt->length)) != PKT_OK){
        fprintf(stderr,("Incorrect payload\n"));
        return st;
    }
    cur += pkt->length;

    memcpy(&(pkt->crc2),data + cur, sizeof(uint32_t));
    cur += 4;
    pkt->crc2 = ntohl(pkt->crc2);

    if(tr == 0 && pkt->length != 0){
        uint32_t crc2_test = crc32(crc32(0L,Z_NULL,0),(Bytef*) data + 12,pkt->length);
        if(pkt->crc2 != crc2_test){
            fprintf(stderr,"Not equals crc2\n");
            return E_CRC;
        }
    }
    pkt_set_tr(pkt,tr);

    if(pkt->type != PTYPE_DATA && pkt->tr != 0){
        fprintf(stderr,"PTYPE_DATA != 0 && tr != 0\n");
        return E_UNCONSISTENT;
    }
    
    return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
    if(pkt == NULL || ((pkt_get_type(pkt) != PTYPE_DATA) && pkt_get_tr(pkt))) { // condition sur le champ TR
        return E_UNCONSISTENT;
    }
    if(buf == NULL) {
        return E_NOMEM;
    }
    if(len == NULL) {
        return E_LENGTH;
    }

    uint8_t head;
    int cur = 0;
    head = (pkt->type  << 6) | (pkt->tr << 5) | (pkt->window & 0x1F) ; 

    memcpy(buf + cur, &head, sizeof(uint8_t));
    cur += 1;

    if(pkt->type != PTYPE_DATA){
        memcpy(buf+cur,&(pkt->seqnum),sizeof(uint8_t)); 
        cur += 1;
        memcpy(buf+cur,&(pkt->timestamp),sizeof(uint32_t));
        cur += 4;
        uint32_t crc1 = ntohl(crc32(crc32(0L,Z_NULL,0),(Bytef *)buf,(uInt)sizeof(char)*cur));
        memcpy(buf+cur,&(crc1),sizeof(uint32_t));
        cur += 4;
        *len = cur;
        return PKT_OK;
    }
    
    uint16_t pktn_length = htons(pkt_get_length(pkt));      
    if(pkt_get_length(pkt) > MAX_PKT_SIZE){
        fprintf(stderr,"Error length : %d\n",pktn_length);
        return E_LENGTH;
    }

    memcpy(buf + cur , &pktn_length, sizeof(uint16_t));
    cur += 2;

    memcpy(buf + cur, &(pkt->seqnum), sizeof(uint8_t));
    cur += 1;

    uint32_t pkt_timestamp = pkt_get_timestamp(pkt);
    memcpy(buf + cur, &pkt_timestamp, sizeof(uint32_t));
    cur += 4;

    uint32_t pktn_crc1 = htonl((uint32_t) crc32(crc32(0L, Z_NULL, 0), (Bytef *) buf + 0, (uInt) sizeof(char) * 8));
    memcpy(buf + cur, &pktn_crc1, sizeof(uint32_t));
    cur += 4;

    if(pkt_get_tr(pkt)) {
        if(pkt_get_length(pkt) != 0) {
            return E_UNCONSISTENT;
        }
        return PKT_OK;
    }

    if(pkt_get_payload(pkt) != NULL && pkt_get_tr(pkt) == 0) {
        memcpy(buf + cur, pkt->payload, pkt_get_length(pkt));
        uint32_t pktn_crc2 = htonl((uint32_t) crc32(crc32(0L, Z_NULL, 0), (Bytef *) buf + cur, (uInt) pkt_get_length(pkt)));
        cur += pkt_get_length(pkt);
        
        if(pktn_crc2 != 0 && pkt_get_tr(pkt) == 0) {
            memcpy(buf + cur, &pktn_crc2, sizeof(uint32_t));
            cur += 4;
        } else {
            return E_UNCONSISTENT;
        }
    }

    *len = cur;
    return PKT_OK;
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
    if(pkt->length)
        return pkt->payload;
    return NULL;
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
    if(tr > 1){
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
    if(length > MAX_PAYLOAD_SIZE) 
        return E_LENGTH;


    pkt->payload = (char*) malloc(sizeof(char)*length);
    if(!pkt->payload)
        return E_NOMEM;
    
    if(pkt_set_length(pkt,length) != PKT_OK)
        return E_LENGTH;
    
    memcpy(pkt->payload,data,length);

    return PKT_OK;
}

ssize_t predict_header_length(const pkt_t *pkt)
{
    if(pkt->type == PTYPE_DATA){
        if(pkt->length > MAX_PAYLOAD_SIZE)
            return -1;

        return (ssize_t)8;
    }else{
        if(pkt->length)
            return -1;
    }
    return (ssize_t)10;
}

void pkt_print(pkt_t* pkt){
	printf("type: %d\ntruncated: %d\nwindow: %d\nlength: %d\nseqnum %d\ntimestamp: %d\ncrc1: %d\npayload: %s\n",
		pkt->type, pkt->tr, pkt->window, pkt->length,pkt->seqnum,pkt->timestamp, pkt->crc1, pkt->payload);
}