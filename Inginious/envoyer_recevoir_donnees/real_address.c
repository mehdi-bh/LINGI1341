#include "real_address.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
/* Resolve the resource name to an usable IPv6 address
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 *          (const char* means the caller cannot modify or free the return value,
 *           so do not use malloc!)
 */
const char * real_address(const char *address, struct sockaddr_in6 *rval){
    struct addrinfo hint;
    struct addrinfo* result;

    memset(&hint,0,sizeof(hint));

    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_family = AF_INET6;
    hint.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(address,NULL,&hint,&result);
    if(s!=0){
        
        return gai_strerror(s);
    }

    memcpy(rval, result->ai_addr, sizeof(struct sockaddr_in6)); // copying the values to rval
    freeaddrinfo(result);
    return NULL;
}
