#include "record.h"


int record_init(struct record *r)
{
    r->type = 0;
    r->has_footer = 0;
    r->length = 0;
    
    r->data = NULL;
    
    return 0;
}

void record_free(struct record *r)
{
    if(r->data != NULL)
        free(r->data);
}

int record_get_type(const struct record *r)
{
    return r->type;
}

void record_set_type(struct record *r, int type)
{
    r->type = type;
}

int record_get_length(const struct record *r)
{
    return r->length;
}

int record_set_payload(struct record *r, const char * buf, int n)
{
    char* data = (char*)malloc(sizeof(char)*n);
    if(!data)
        return -1;
    
    memcpy(data, buf, n);
    
    r->length = n;
    if(r->data)
        free(r->data);
    r->data = data;

    return 0;
}

int record_get_payload(const struct record *r, char *buf, int n)
{
    int value;
    if(r->length < n){
        value = r->length;
    }
    else{
        value = n;
    }
    
    memcpy(buf, r->data, value);
    
    return value;
}

int record_has_footer(const struct record *r)
{
    return r->has_footer;
}

void record_delete_footer(struct record *r)
{
    r->has_footer = 0;
    r->UUID = 0;
}

void record_set_uuid(struct record *r, unsigned int uuid)
{
    if(!r->has_footer){
        r->has_footer = 1;
    }
    r->UUID = uuid;
}

unsigned int record_get_uuid(const struct record *r)
{
    return r->UUID;
}

int record_write(const struct record* r, FILE *f){
    uint16_t l = htons(r->length);
    int sum = 0;
    uint16_t type = r->type << 1;
    if(r->has_footer){
     	type += 1;   
    }
    type = htons(type);
    sum += sizeof(uint16_t) * fwrite(&type,sizeof(uint16_t),1,f);
    sum += sizeof(uint16_t) * fwrite(&l,sizeof(uint16_t),1,f);
    
    if(r->length > 0){
        sum += sizeof(char)*r->length*fwrite(r->data,r->length*sizeof(char),1,f);
    }
    if(r->has_footer){
        uint32_t uuid = r->UUID;
        sum += sizeof(uint32_t)*fwrite(&uuid,sizeof(uint32_t),1,f);
    }
    return sum;
}
int record_read(struct record *r, FILE *f){
    int sum = 0;
    
    uint16_t type;
    sum += fread(&type,1,sizeof(uint16_t),f);
	type = ntohs(type);
    int hasF = type & 1;
    r->has_footer = hasF;
    r->type = type >> 1;
                 
    uint16_t l;
    sum += fread(&l,1,sizeof(uint16_t),f);
    r->length = ntohs(l);
    
    if(r->length > 0){
        char* d = malloc(sizeof(char)*r->length);
    	if(!d) return -1;
    	sum += fread(d,1,sizeof(char)*r->length,f);
        record_set_payload(r,d,r->length);
    }else{r->data = NULL;}
    
    uint32_t footer;
    if(r->has_footer){
        sum += fread(&footer,1,sizeof(uint32_t),f);
        record_set_uuid(r,footer);
    }else{
        r->UUID = 0;
    }
    
    return sum;
}

int main(int argc, char* argv[]){
	
	system("rm test.dms");
	system("touch test.dms");
	struct record* lector = (struct record*)malloc(sizeof(struct record));
	record_init(lector);
	char* str = "abcdef0123456789bbcc";
	lector->type = 23;
	lector->has_footer = 1;
	lector->length = 10;

	record_set_payload(lector, str,21);
	//printf("%s\n", r->data);
	record_set_uuid(lector, 1234);
	FILE *f = fopen("test.dms", "w");
    record_write(lector,f);
	//struct record lector;
	//record_read(&lector, f);
printf("test %d %d %d %s %d",lector->type,lector->has_footer,lector->length,lector->data,lector->UUID);

}