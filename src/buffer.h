
/*
 * $Revision: 1.7 $
 * $Author: sderr $
 * $Date: 2002/06/07 07:58:49 $
 * $Header: /cvsroot/ka-tools/ka-deploy/src/buffer.h,v 1.7 2002/06/07 07:58:49 sderr Exp $
 * $Id: buffer.h,v 1.7 2002/06/07 07:58:49 sderr Exp $
 * $Log: buffer.h,v $
 * Revision 1.7  2002/06/07 07:58:49  sderr
 * Improved end-of-buffer flushing in the client [done in the main select() loop now]
 *
 * Revision 1.6  2002/03/15 10:46:09  sderr
 * Added names to the buffers
 *
 * Revision 1.5  2001/11/15 16:44:18  sderr
 * -minor cleanups
 * -fixed TWO silly bugs around my old 'unsigned long long' -> introduced offset_t type, see in buffer.h
 *
 * Revision 1.4  2001/11/14 15:41:18  sderr
 * Now the last client of the chain tells the server from time to time
 * what data it has, and so the server
 * can make sure he will have the necessary data in case of failure
 * (that is all the report_pos) stuff
 * -still only for the chain
 * -if we lose the last client, transfer stops (should be easy to fix)
 *
 * Revision 1.3  2001/11/14 10:34:57  sderr
 * Many many changes.
 * -silly authentication done by client/server communication on data connections
 * -it was needed for this : recovery when one client fails. It takes a lot of changes
 * This seems to work, but :
 * 	* error detection : well, it's ok when I kill -INT a client, that's all
 * 	* sometimes the node re-contacted does not have the data the re-contacting node needs -- needs to be fixed
 * 	* works only in the arity = 1 case.
 *
 * Revision 1.2  2001/11/09 14:37:37  sderr
 * Major changes on the server, which uses also the data buffer now.
 * Cleanups on the server, the filedes are -at last- not used anymore as array indexes (i.e. much crap removed)
 * I think I can expect a performance boost, but also a higher CPU utilization.
 *
 * Revision 1.1  2001/11/09 10:15:31  sderr
 * Some cleanups, mostly in client.c
 * Moved the data_buffer stuff to buffer.c and buffer.h so I'll be able to use it on the server also
 * Added the new struct consumer
 *
*/



#ifndef BUFFER_H_INC
#define BUFFER_H_INC

#include <sys/types.h>

#ifdef VERY_SMALL_BUFFER

// debug buffer here !
#define DATA_BUFFER_SIZE (100)
#define WANT_FREE_SPACE 10
#define SIZE_TO_SEND 60
#define SIZE_WHEN_SEND 50
// buffer for reading : 
#define BUFFERSIZE 9
#else

// real buffer size:
// big buffer
#define DATA_BUFFER_SIZE (( 1 << 20 ) * 35 )
#define SIZE_TO_SEND 32678
#define SIZE_WHEN_SEND 32768
#define BUFFERSIZE 32768
#define WANT_FREE_SPACE (2 * BUFFERSIZE)
#endif

/* NOTE : offset_t is SIGNED because it may be negative on early calls to max_off in server.c
 */
typedef long long offset_t; 

#define MAX_CONSUMERS 10

void init_data_buffer_for(int fd);
void drop_data_buffer_for(int fd);
int has_data_buffer(int fd);
int data_in_data_buffer_for(int z);
int space_left_in_data_buffer();
void store_in_data_buffer(char * what, int size);
int send_from_data_buffer(int fd, int size);
void init_data_buffer();
int add_data_buffer_consumer(int fd);
int data_buffer_consumer_exists(int id);



/** see who are the clients ready == who exist AND have data in the buffer
 * they must have MORE (not at least, MORE) data than min_data
 * */

void data_buffer_get_ready_consumers(fd_set * set, unsigned int min_data);


void data_buffer_send_data_to_set(fd_set * set);
int data_buffer_set_consumer_pos(int id, offset_t offset);
offset_t data_buffer_total_rcv();
offset_t data_buffer_get_startpos();
int still_in_data_buffer(offset_t offset);

void data_buffer_set_name(int id, char * name);
char * data_buffer_get_name(int id);
int is_data_buffer_empty();

#endif
