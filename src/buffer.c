
/*
 * $Revision: 1.8 $
 * $Author: sderr $
 * $Date: 2002/06/07 07:58:49 $
 * $Header: /cvsroot/ka-tools/ka-deploy/src/buffer.c,v 1.8 2002/06/07 07:58:49 sderr Exp $
 * $Id: buffer.c,v 1.8 2002/06/07 07:58:49 sderr Exp $
 * $Log: buffer.c,v $
 * Revision 1.8  2002/06/07 07:58:49  sderr
 * Improved end-of-buffer flushing in the client [done in the main select() loop now]
 *
 * Revision 1.7  2002/03/15 10:46:09  sderr
 * Added names to the buffers
 *
 * Revision 1.6  2001/11/19 13:39:43  sderr
 * Quite many changes, because I wanted to improve the client death detection
 * This has been harder than expected, because now the _server_ can be the one who des the detection
 * --> more client/server exchanges
 * --> after a data connection is opened, auth step now includes a report_position from the child
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
 * Revision 1.1  2001/11/09 10:15:30  sderr
 * Some cleanups, mostly in client.c
 * Moved the data_buffer stuff to buffer.c and buffer.h so I'll be able to use it on the server also
 * Added the new struct consumer
 *
*/



#include "buffer.h"
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>


static int max(int a, int b) { return a > b ? a : b; }
static int min(int a, int b) { return a < b ? a : b; }

/** the offset of the data (i.e. where to begin next read in the buffer) for each consumer. 
 */

struct data_buffer_consumer {
	int pos;
	int fd;
	int exists;
	char * name;
};

struct data_buffer_consumer data_buffer_consumers[MAX_CONSUMERS];

/** the buffer himself */
char * data_buffer;

/** the end of the data in the buffer */
int data_buffer_end;

offset_t data_buffer_total_received;

offset_t data_buffer_get_startpos() { 
	if (data_buffer_total_received <= DATA_BUFFER_SIZE) return 0;
	
	return data_buffer_total_received - DATA_BUFFER_SIZE;
}


/** Add a new consumer -- return consumer ID, or -1 if failure*/
int add_data_buffer_consumer(int fd)
{
	int i;
	for (i = 0; i < MAX_CONSUMERS; i++)
		if (!data_buffer_consumers[i].exists)
		{
			data_buffer_consumers[i].exists = 1;
			data_buffer_consumers[i].pos = 0;
			data_buffer_consumers[i].fd = fd;			
			data_buffer_consumers[i].name = 0;			
			return i;
		}
		
	return -1;
}

void data_buffer_set_name(int i, char * name)
{
	if (data_buffer_consumers[i].name) free(data_buffer_consumers[i].name);
	assert(data_buffer_consumers[i].name = strdup(name));			
}

char * data_buffer_get_name(int i)
{
	return data_buffer_consumers[i].name;
}
	
/* modifies the position of a consumer to the 'offset' value -- taken from the start of the stream
 * returns zero on sucess
 * returns -1 on failure
 */
int data_buffer_set_consumer_pos(int id, offset_t offset)
{
	int pos;
	offset_t data_buffer_startpos = data_buffer_get_startpos();

//	printf("\nData buffer set pos : data_buffer_startpos = %lld"
//	       "\n                                    offset = %lld\n\n", data_buffer_startpos, offset);
//
	if (data_buffer_startpos > offset) return -1;
	if (data_buffer_startpos + DATA_BUFFER_SIZE < offset) 
	{
		printf("-- someone wants data I don't have YET ?\n");
		return -1; // will not happen. I could handle this case -- useless
	}

	if (data_buffer_startpos)
		pos = offset - data_buffer_startpos + data_buffer_end;
	else
		pos = offset;

	if (pos > DATA_BUFFER_SIZE) pos -= DATA_BUFFER_SIZE;

	
	data_buffer_consumers[id].pos = pos;
//	printf(" end = %d, pos = %d\n", data_buffer_end, pos);
	return 0;
}

int still_in_data_buffer(offset_t offset)
{
	offset_t data_buffer_startpos = data_buffer_get_startpos();
	if (data_buffer_startpos > offset) return 0;
	if (data_buffer_startpos + DATA_BUFFER_SIZE < offset) return 0;
	return 1;
}

int data_buffer_consumer_exists(int id)
{
	return data_buffer_consumers[id].exists;
}

offset_t data_buffer_total_rcv()
{
	return data_buffer_total_received;
}

/** close the data buffer -- i.e drop this client*/
void drop_data_buffer_for(int id)
{
	data_buffer_consumers[id].exists = 0;
}


/** Return the amount of data available a consumer in the data buffer */
int data_in_data_buffer_for(int id)
{
	int f;
	
	if (!data_buffer_consumers[id].exists)
		return 0;
		
	f =  data_buffer_end - data_buffer_consumers[id].pos;
	/* wrap around the end of the buffer if necessary */
	f = (f >= 0) ? f : f + DATA_BUFFER_SIZE;
	
/*	printf("For client %d, pos = %d, end = %d, size = %d\n", z, data_buffer_consumers[z].pos, data_buffer_end, f); */
	
	return f;
}

/** see if data buffer is empty
 * returns 1 if empty
 * returns 0 if not
 * */
int is_data_buffer_empty()
{
	int z;
	for (z=0; z < MAX_CONSUMERS;z++)
		if (data_buffer_consumers[z].exists)
		{
			if (data_in_data_buffer_for(z) > 0) return 0;
		}
	
	return 1;
}

/** Return the free space in the data buffer */
int space_left_in_data_buffer()
{
	int z;
	int cmax = 0;
	for (z=0; z < MAX_CONSUMERS;z++)
		if (data_buffer_consumers[z].exists)
		{
			cmax = max (cmax, data_in_data_buffer_for(z));
		}
	
	return DATA_BUFFER_SIZE - cmax;
}

/** Add data into the data buffer 
  * There MUST be enough free space (not checked) 
  */
  
void store_in_data_buffer(char * what, int size)
{
	int chunk1, chunk2;
	
/*	printf("Storing %d bytes in the buffer\n", size); */

	/** we check if we have to wrap around, when meeting the end of the physical buffer */
	chunk1 = min (size, DATA_BUFFER_SIZE - data_buffer_end);
	chunk2 = max (size - chunk1, 0);

	memcpy(data_buffer + data_buffer_end, what, chunk1);
	if (chunk2) memcpy(data_buffer, what + chunk1, chunk2);

	/*
	 * ok, I have to explain a bit :
	 * If the buffer has already wrapped at least once, then we are sure that the startpos is increased by 'size'
	 * If the buffer has not yet wrapped :
	 * - if we do not wrap, the startpos remains at zero
	 * - if we do wrap, the startpos equals the amount of data overwritten = data_buffer_end
	 */

	data_buffer_end += size;
	data_buffer_total_received += size;
	if (data_buffer_end >= DATA_BUFFER_SIZE) 
		// the buffer wraps around
		data_buffer_end -= DATA_BUFFER_SIZE;
}

/** For filedes fd, and customer id 'id' take _size_ data in the buffer, and write() them to fd */
int send_from_data_buffer(int id, int size)
{
	int chunk1, chunk2;
	int pos = data_buffer_consumers[id].pos;
	int fd = data_buffer_consumers[id].fd;
	int w1 = -1;
	int w2 = -1;
	
/*	printf("Sending %d bytes from the buffer to %d\n", size, fd);	*/

	/* we check if we have to wrap around, when meeting the end of the physical buffer */
	size = min (size, data_in_data_buffer_for(id));
	chunk1 = min (size, DATA_BUFFER_SIZE - pos);
	chunk2 = max (size - chunk1, 0);

	/* socket may be non blocking beware 
	 */
	w1 = write(fd, data_buffer + pos, chunk1);
	if (w1 == chunk1)
	if (chunk2) 
		w2 = write(fd, data_buffer, chunk2);
	
	if (w1 > 0) pos += w1;
	if (w2 > 0) pos += w2;

	pos = (pos >= DATA_BUFFER_SIZE) ? pos - DATA_BUFFER_SIZE : pos ;
	
	data_buffer_consumers[id].pos = pos;
	
	return (chunk1 + chunk2);
}


void init_data_buffer()
{
	int i;
	assert(data_buffer = (char *)malloc(DATA_BUFFER_SIZE));
	data_buffer_end = 0;
	for (i=0; i < MAX_CONSUMERS;i++) {
		data_buffer_consumers[i].exists = 0;
	}
	data_buffer_total_received = 0;
}

void data_buffer_get_ready_consumers(fd_set * set, unsigned int min_data)
{
	int i;
	for (i=0; i < MAX_CONSUMERS; i++)
	{
		int fd;
		if (!data_buffer_consumers[i].exists) continue;
		fd = data_buffer_consumers[i].fd;
		if (data_in_data_buffer_for(i) > min_data)
		{
			FD_SET(fd, set);
			// hmm, I also want to know if there is sthg to read on those sockets -- it means ERROR ERROR
		}
	}	
}


void data_buffer_send_data_to_set(fd_set * set)
{
	int i;
	for (i=0; i < MAX_CONSUMERS; i++)
	{
		int fd;
		if (!data_buffer_consumers[i].exists) continue;
		fd = data_buffer_consumers[i].fd;
		if (FD_ISSET(fd, set))
			send_from_data_buffer(i, SIZE_WHEN_SEND);
	}	
}
