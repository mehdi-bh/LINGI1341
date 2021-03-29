#include <netinet/in.h>
#include <sys/types.h>

#ifndef __READ_WRITE_LOOP_H_
#define __READ_WRITE_LOOP_H_

/* 
 * Function:  wait_for_client 
 * --------------------------
 * Used by the receiver to connect to the client through socket.
 * @sfd:        Socket file descriptor created with create_socket().
 * @return:     0   if success.
 *             -1   if an error occurred.
 */
int wait_for_client(int sfd);

/* 
 * Function:  read_address 
 * -----------------------
 * Resolve the resource name to an usable IPv6 address
 * @address:    The name to resolve
 * @rval:       Where the resulting IPv6 address descriptor should be stored
 * @return:     NULL if it succeeded, or a pointer towards
 *              a string describing the error if any.
 *              (const char* means the caller cannot modify or free the return value,
 *              so do not use malloc!)
 */
const char * real_address(const char *address, struct sockaddr_in6 *rval);

/* 
 * Function:  create_socket 
 * ------------------------
 * Creates a socket and initialize it
 * @source_addr: if !NULL    the source address that should be bound to this socket
 * @src_port:    if > 0      the port on which the socket is listening
 * @dest_addr:   if !NULL    the destination address to which the socket should send data
 * @dst_port:    if > 0      the destination port to which the socket should be connected
 * @return:      (int)       a file descriptor number representing the socket,
 *               -1          in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port);

#endif
