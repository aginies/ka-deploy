

/*
 * $Revision: 1.27 $
 * $Author: sderr $
 * $Date: 2002/06/07 08:19:56 $
 * $Header: /cvsroot/ka-tools/ka-deploy/src/client.c,v 1.27 2002/06/07 08:19:56 sderr Exp $
 * $Id: client.c,v 1.27 2002/06/07 08:19:56 sderr Exp $
 * $Log: client.c,v $
 * Revision 1.27  2002/06/07 08:19:56  sderr
 * Did the same end-of-flow buffer flushing as in the client [in main select()]
 *
 * Revision 1.26  2002/06/07 07:58:49  sderr
 * Improved end-of-buffer flushing in the client [done in the main select() loop now]
 *
 * Revision 1.25  2002/06/06 14:16:22  sderr
 * changed usec variable to long long to avoid negative times
 *
 * Revision 1.24  2002/05/31 08:52:36  sderr
 * TCP ports can now be changed with -p and -P options
 *
 * Revision 1.23  2002/03/15 10:45:48  sderr
 * Buffer consumers are now non blocking
 *
 * Revision 1.22  2001/11/19 13:39:43  sderr
 * Quite many changes, because I wanted to improve the client death detection
 * This has been harder than expected, because now the _server_ can be the one who des the detection
 * --> more client/server exchanges
 * --> after a data connection is opened, auth step now includes a report_position from the child
 *
 * Revision 1.21  2001/11/16 11:32:34  sderr
 * -Clients now send keep-alive packets to their daddy when they are inactive
 *
 * Revision 1.20  2001/11/15 16:44:18  sderr
 * -minor cleanups
 * -fixed TWO silly bugs around my old 'unsigned long long' -> introduced offset_t type, see in buffer.h
 *
 * Revision 1.19  2001/11/14 15:41:18  sderr
 * Now the last client of the chain tells the server from time to time
 * what data it has, and so the server
 * can make sure he will have the necessary data in case of failure
 * (that is all the report_pos) stuff
 * -still only for the chain
 * -if we lose the last client, transfer stops (should be easy to fix)
 *
 * Revision 1.18  2001/11/14 10:34:57  sderr
 * Many many changes.
 * -silly authentication done by client/server communication on data connections
 * -it was needed for this : recovery when one client fails. It takes a lot of changes
 * This seems to work, but :
 * 	* error detection : well, it's ok when I kill -INT a client, that's all
 * 	* sometimes the node re-contacted does not have the data the re-contacting node needs -- needs to be fixed
 * 	* works only in the arity = 1 case.
 *
 * Revision 1.17  2001/11/09 14:34:54  sderr
 * Few bugfixes, data_buffer interface updated.
 *
 * Revision 1.16  2001/11/09 10:15:31  sderr
 * Some cleanups, mostly in client.c
 * Moved the data_buffer stuff to buffer.c and buffer.h so I'll be able to use it on the server also
 * Added the new struct consumer
 *
 * Revision 1.15  2001/09/13 14:59:15  sderr
 * Added an option (-w) which says to the clients that they have to wait
 * until all clients have finished the transfer before exiting.
 *
 * Revision 1.14  2001/08/29 06:36:13  mikmak
 * correcting missing option
 *
 * Revision 1.13  2001/08/17 14:20:52  mikmak
 * performance improvement for multicast support and updated usage/help
 *
 * Revision 1.12  2001/07/02 11:39:13  sderr
 * Moved things so that:
 * 	-multicast support is optional (check Makefile)
 * 	-.c files again can be compiled using gcc, not g++ (added multicast_server.cpp and multicast_client.cpp)
 * Cleaned transport_mode variable
 *
 * Revision 1.11  2001/07/01 15:27:18  mikmak
 * multicast is now integrated and working. Many updates to rmtplib and optimization for Ka.Still much to do ...
 *
 * Revision 1.10  2001/06/29 09:31:45  sderr
 * *** empty log message ***
 *
 * Revision 1.9  2001/06/05 08:51:57  sderr
 * Added [ -e command ] command line option
 *
 * Revision 1.8  2001/05/30 15:00:08  sderr
 * Client now checks udp reply against version string
 *
 * Revision 1.7  2001/05/30 14:35:51  sderr
 * Added udp support, clients now can find the server by sending UDP broadcasts
 *
 * Revision 1.6  2001/05/30 13:35:08  sderr
 * Added getopt use for command-line parsing
 *
 * Revision 1.5  2001/05/21 10:03:02  sderr
 * Re-added support for operation without a data buffer. Hope I broke nothing. Seems OK.
 *
 * Revision 1.4  2001/05/10 12:57:51  sderr
 * Removed assert() when trying to connect, put a perror() instead
 *
 * Revision 1.3  2001/05/10 12:18:02  sderr
 * Added comments, fixed open comment on CVS header
 * Function renaming in client
 *
 * Revision 1.2  2001/05/03 12:34:41  sderr
 * Added CVS Keywords to most files. Mostly useless.
 *
 * $State: Exp $
 */


#include "packet.h"
#include "udp.h"
#include "netdb.h"
#include <assert.h>
#include <errno.h>
#include "tree.h"
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
#include "buffer.h"
#include <fcntl.h>
#include <unistd.h>


static char version[] = SOFT_VERSION;

#define PRINT_INFO
#define PRINT_WARN
//#define PRINT_DEBUG


#ifdef PRINT_DEBUG
 #define debug(args...) fprintf(stderr, args)
#else
 #define debug(args...) 
#endif


#ifdef PRINT_INFO
 #define info(args...) fprintf(stderr, args)
#else
 #define info(args...) 
#endif

#ifdef PRINT_WARN
 #define warn(args...) fprintf(stderr, args)
#else
 #define warn(args...)
#endif


//#define debug(args...) 
//#define info(args...) 

//#define VERY_SMALL_BUFFER

#define USE_DATA_BUFFER

//#define EXTCOMMAND "sed s/bytes/mein/g"
//#define EXTCOMMAND "tr i X"
//#define EXTCOMMAND "wc"
//#define EXTCOMMAND "cat"
//#define EXTCOMMAND "cat |cat |cat |cat |cat |cat |cat |cat |cat |cat |cat |cat | tr A B | wc"
//#define EXTCOMMAND "(cd /tmp/disk; tar tfvB -)"

//#define EXTCOMMAND "(cd /tmp/disk; tar --exclude / -xfvpB  -)"
//#define EXTCOMMAND "(cd /tmp/disk; tar --extract  --read-full-records --same-permissions --numeric-owner --sparse --file - )"
#define EXTCOMMAND "(cd /tmp/stage2; tar --x -f - )"


//#define SERVER "icluster17"
//#define SERVER "ute3"
//#define SERVER "129.88.69.201"

int wait_others = 0;

char * ext_command = EXTCOMMAND;

int socket_command = -1; // connected to the server
int socket_server = -1; // for incoming connections
int socket_data = -1; // for incoming data

int fd_max;

/* we have finished RECEIVING data */
int flow_ended = 0;

/* we have finished SENDING data */
int children_finished = 0;


/* we must report our data postition to the server */
int must_report_position = 0;

/* min data in the buffer to call write() */
unsigned int send_thresold = SIZE_TO_SEND;

struct child
{
	int exists;
	int auth;
	int fd;
	int consumer_id; // for the data buffer
	IP ip;
	int got_greeting;
};

struct child children[ARITE_MAX];
int nb_children = 0;

IP dad_ip;

#define MAX_CONN (ARITE_MAX+10)

void start_external_command();
int command_started = 0;
int pipe_read = -1;
int pipe_write = -1;
int pipe_consumer_id;

static int max(int a, int b) { return a > b ? a : b; }
//static int min(int a, int b) { return a < b ? a : b; }



/*
 * Notes about 'the data buffer'
 * When a client receives data, ie will put them is his data buffer.
 * Then 2 or more different consumers will want those data :
 * - the external command
 * - the network connection(s) to the child(ren)
 *
 * This buffer is a circular buffer. 
 * We will accept data from the network only when this buffer is not full, 
 * and we wait for it to hold a certain amount of data to send it.
 * The goal of this buffer is to be able to send the data through the network even if we have not yet written it to the disk.
 * But in fact since Linux has its own buffer (disk cache) the effect is almost void...
 * Anyway, this buffer may help in the future If we want to try to do some error recovery.
 */

/* stuff for the buffer */





/* the buffer which will hold the data before we store them in the 'data buffer' */
static char read_buffer[BUFFERSIZE];

int process_packet(struct server_packet * packet);

#ifdef USE_DATA_BUFFER

#ifdef USE_RMTP_LIB
/* Multicast address */
IP mcast;
int ack_interval=0;

/* from multicast_server.cpp : */
extern  void multicast_main_loop();

#endif
enum { TCP_TREE_TRANS, MULTICAST_TRANS } transport_modes;

/* Transport mode */
int transport_mode = TCP_TREE_TRANS; 



#endif /* USE_DATA_BUFFER */


unsigned short tcp_port_comm = PORTNUMCOMM;
unsigned short tcp_port_data = PORTNUMDATA;

/* IPv4, 32bit only HACK !!!!! */
/** same_ip : returns true if a and b are the same IP address */
int same_ip(IP a, IP b)
{
	int *x = (int *) &a;
	int *y = (int *) &b;
	return *x == *y;
}


int send_packet_server(struct client_packet * packet)
{
	return write(socket_command, packet, sizeof(struct client_packet));
}

int say_hello()
{
	struct client_packet packet;
	struct packet_client_says_hello * pdata;
	
	packet.type = client_says_hello;
	pdata = (struct packet_client_says_hello *) &(packet.data);
	
	strcpy(pdata->version, version);
	
	return send_packet_server(&packet);	
}

int send_ping_reply()
{
	struct client_packet packet;
	
	packet.type = client_ping_reply;
	
	return send_packet_server(&packet);	
}

int say_go_now()
{
	struct client_packet packet;
	
	packet.type = client_says_go_now;
	
	return send_packet_server(&packet);	
}

int say_has_finished()
{
	struct client_packet packet;
	
	packet.type = client_says_has_finished;
	
	return send_packet_server(&packet);	
}

int say_accept_data()
{
	struct client_packet packet;
	
	packet.type = client_accepts_data;
	
	return send_packet_server(&packet);	
}


int send_options()
{
	struct client_packet packet;
	struct packet_client_sends_options * pdata;
	
	packet.type = client_sends_options;
	pdata = (struct packet_client_sends_options *) &(packet.data);
	pdata->will_wait_for_others = wait_others;
	
	return send_packet_server(&packet);	
}


int report_position()
{
	struct client_packet packet;
	struct packet_client_reports_position * pdata;
	
	packet.type = client_reports_position;
	pdata = &(packet.data.report_pos);
	
	pdata->data_received = data_buffer_total_rcv();
	
	return send_packet_server(&packet);	
}

int say_dad_disconnected()
{
	struct client_packet packet;
	struct packet_client_says_dad_disconnected * pdata;
	
	packet.type = client_says_dad_disconnected;
	pdata = &(packet.data.disconn);
	
	pdata->data_received = data_buffer_total_rcv();
	pdata->dad_ip = dad_ip;
	debug("Lost connection with %s at position %lld\n", inet_ntoa(dad_ip), pdata->data_received);
	
	return send_packet_server(&packet);	
}

int say_i_got_a_client(IP ip)
{
	struct client_packet packet;
	struct packet_client_got_client * pdata;
	
	debug("say_i_got_a_client(%s)\n", inet_ntoa(ip));
	packet.type = client_got_client;
	pdata = &(packet.data.got_client);
	
	pdata->child_ip = ip;
		
	return send_packet_server(&packet);	
}

		

int get_ip_of(char * hostname, IP * ip)
{
	struct hostent * he;
	
	he = gethostbyname(hostname);
	if (he == NULL) return -1;
	assert(sizeof(IP) == he->h_length);
	memcpy((char *) ip, he->h_addr, he->h_length);
	return 0;
}


int call_port(IP ip, unsigned short port)
{
	struct sockaddr_in sa;
	int e;
	int sock;


	memset(&sa, 0, sizeof(struct sockaddr_in));

	sa.sin_addr = ip;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port); 

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) return -1;

	e = connect(sock, (struct sockaddr *) &sa, sizeof(struct sockaddr_in));
	if (e < 0) 
	{
		close(sock);
		return -1;
	}

	debug("Socket %d connected to port %d on %s.\n", sock, port, inet_ntoa(ip));

	return sock;
}

int close_connection(int fd)
{
	close(fd);
	return -1;
}

/** Read a packet on a control connection */
int read_packet_from_server(int socket)
{
	struct server_packet packet;
	int n;
	
	debug("Reading control packet from socket %d\n", socket); 
	n = read(socket, (void *) &packet, sizeof(struct server_packet));
//	debug("%d bytes\n", n); 
	/* don't bother, my packets are small and should be read in exactly one try */
	if (n!= sizeof(struct server_packet))
	{
		if (n>0) warn("Wrong packet size on socket %d\n", socket);
		if (n==0) warn("Peer closed connection on socket %d\n", socket);
		if (n<0) warn("Error on socket %d : %s\n", socket, strerror(errno));
		close_connection(socket);
		return -1;
	}
		
	return process_packet(&packet);
}

/** find a free struct child */
struct child * new_child()
{
	int i;
	for (i=0; i < ARITE_MAX; i++)
		if (!children[i].exists) return &children[i];

	return 0;
}

/** find a struct child */
struct child * find_child(IP cip)
{
	int i;
	for (i=0; i < ARITE_MAX; i++)
		if ((children[i].exists) && (same_ip(children[i].ip, cip))) return &children[i];

	return 0;
}




/** Accept a child = a new outgoing data connection */
void accept_new_data_connection()
{
	struct sockaddr_in client_addr;
	socklen_t client_addr_size;
	int socket_client;
	struct child * son;
	IP ip;
	long orig;

	memset(&client_addr,0, sizeof(client_addr));
	client_addr_size = sizeof(client_addr);
	socket_client = accept(socket_server, (struct sockaddr *) &client_addr, &client_addr_size);
	ip = client_addr.sin_addr;
	
	info("Accepting connection from %s, socket = %d\n", inet_ntoa(ip), socket_client);
	
	if (socket_client >= MAX_CONN)
	{
		warn("uh, oh, connection with fd = %d\n", socket_client);
		close(socket_client);
		return;
	}
	
	if (nb_children == ARITE_MAX)
	{
		warn("Hey, I already have %d children!\n", nb_children);
		close(socket_client);
		return;
	}
		
	
	say_i_got_a_client(ip);
	// do not really accept the child -- wait until the server confirms 
	son = new_child();
	assert(son);
	son->exists = 1;
	son->auth = 0;
	son->fd = socket_client;
	orig = fcntl(socket_client, F_GETFL);
 	fcntl(socket_client, F_SETFL, orig | O_NONBLOCK);
	son->ip = ip;
	info("Got new data connection, awaiting server auth...\n");
}

/* we got an auth packet from the server for this client */
void really_accept_child(IP cip, offset_t data_offset)
{
	int err;
	struct child * son = find_child(cip);
	assert(son);

	if (son->fd > fd_max) fd_max = son->fd;
	son->auth = 1;

	info("Welcome son, you are number %d (MAX %d)\n", nb_children, ARITE_MAX);
	
#ifdef USE_DATA_BUFFER	
	son->consumer_id = add_data_buffer_consumer(son->fd);
	assert(son->consumer_id >= 0);
	data_buffer_set_name(son->consumer_id, inet_ntoa(cip));
	debug("Setting consumer position at %lld\n", data_offset);
	err = data_buffer_set_consumer_pos(son->consumer_id, data_offset);
	if (err < 0)
	{
		warn("Error : client wants data i do not have !\n");
		abort();
	}
#endif

	//tell the server
	say_i_got_a_client(son->ip);

}

void drop_child(struct child * c)
{
	close(c->fd);
	drop_data_buffer_for(c->consumer_id);
	c->exists = 0;
	warn("Dropping client %s\n", inet_ntoa(c->ip));
}

int process_server_says_go(struct packet_server_says_go * p)
{
	debug("Server says go\n");
	return 0;
}


int process_server_says_finish(struct packet_server_says_finish * p)
{
	debug("Other clients have finished, exiting...\n");
	wait_others = 0;
	return 0;
}


int process_server_says_drop_child(struct packet_server_says_drop_child * p)
{
	struct child * victim = find_child(p->ip);
	if (!victim) return 0;


	debug("Server requests : drop %s\n", inet_ntoa(p->ip));
	drop_child(victim);

	return 0;
}


int process_server_gives_auth(struct packet_server_auth_child * p)
{
	debug("Server gives auth for %s, offset = %lld\n", inet_ntoa(p->ip), p->data_offset);
	really_accept_child(p->ip, p->data_offset);
	return 0;
}

int process_server_says_please_report_position(struct packet_server_says_please_report_position * p)
{
	if (p->yes_or_no == rp_once)
		report_position();
	else
	{
		must_report_position = p->yes_or_no;
		if (must_report_position) debug("Server wants us to report our position\n");
	}
	return 0;
}


/** The server gives the IP of our 'daddy' */
int process_server_gives_ip(struct packet_server_gives_ip * p)
{
	debug("Server gives IP : %s\n", inet_ntoa(p->ip));
	

	if (socket_data != -1)
	{
		debug("Dropping current data connection\n");
		close(socket_data);
		socket_data = -1;
	}

	/* call daddy : open data connection with him */
	socket_data = call_port(p->ip, tcp_port_data);
	if (socket_data < 0) 
	{
		perror("connect");
		assert(0);
	}
	fd_max = max(socket_data, fd_max);		
	dad_ip = p->ip;

	
	/* start command */
	if (!command_started) start_external_command();
	
	return 0;
}

void process_end_of_data();

int process_server_says_end_of_flow()
{
	debug("Server says end-of-flow\n");
	process_end_of_data();
	return 0;
}


int process_server_sends_ping()
{
	debug("Server sends ping\n");
	send_ping_reply();
	return 0;
}

/** Process a control packet that has been received and read */
int process_packet(struct server_packet * packet)
{
	switch(packet->type)
	{
		case server_says_go:
			return process_server_says_go((struct packet_server_says_go *) &(packet->data));
		case server_says_finish:
			return process_server_says_finish((struct packet_server_says_finish *) &(packet->data));
		case server_gives_ip:
			return process_server_gives_ip((struct packet_server_gives_ip *) &(packet->data));
		case server_says_drop_child:
			return process_server_says_drop_child((struct packet_server_says_drop_child *) &(packet->data));
		case server_says_please_report_position:
			return process_server_says_please_report_position((struct packet_server_says_please_report_position *) &(packet->data));
		case server_gives_auth:
			return process_server_gives_auth((struct packet_server_auth_child *) &(packet->data));
		case server_says_end_of_flow:
			return process_server_says_end_of_flow();
		case server_sends_ping:
			return process_server_sends_ping();
			
	}
	warn("Server sent an unknown command\n");
	return -1;
}

/** A few vars to know the througput */
offset_t totaldata = 0;
offset_t lasttotal = 0;
offset_t lastreport = 0;
unsigned int datapackets = 0;
struct timeval data_time;
struct timeval last_time;

/** We have read data on the data connection, do something with it */
void process_data(char * what, int size)
{
	struct timeval now;
	long long usec;
	float sec;
	float rate;
	
	
	if (datapackets == 0) /* first data arrival, start clock */
	{
		int z;
		gettimeofday(&data_time, 0);
		memcpy(&last_time, &data_time, sizeof(struct timeval));
#ifdef USE_DATA_BUFFER
		info("Buffers names :");
		for (z=0; z < MAX_CONSUMERS;z++)
		if (data_buffer_consumer_exists(z))
		{
			char * s = data_buffer_get_name(z);
			info("%s ", s ? s : "??");
		}
#endif
		info("\n -- No stats yet -- ");
	}
	
#ifdef USE_DATA_BUFFER

	/* put data in the buffer - they will be sent later - after the next select() - maybe very soon, */	
	store_in_data_buffer(what, size);
	
#else /* ! USE_DATA_BUFFER */

	/* send data to network children */
	for (i=0; i < ARITE_MAX;i++) 
		if (children[i].exists)
			write(children[i].fd, read_buffer, size);
	
	/* send data to local child */
	if (pipe_write != -1) write(pipe_write, what, size);
	
#endif /* ! USE_DATA_BUFFER */

	
	
	totaldata += size;
	// each meg if necessary report our position to the server
	if ((totaldata - lastreport) >  (1 << 20)) /* for each meg */
	{
		if (must_report_position) report_position();
		lastreport = totaldata;
	}

#ifdef OLD_STATUS
	/*
	 * I don't print status and report at the same time : I need to report often, but If I print too often
	 * the throughput calculation does not work very well
	 */
	if ((datapackets++ & 0xff) == 0) /* for each 256 reads() */
	if ((totaldata - lasttotal) >  (1 << 20)) /* for each meg */
	{
		int z;
		gettimeofday(&now, 0);		
		usec = now.tv_sec - last_time.tv_sec;
		usec = (usec * 1000000) + (now.tv_usec - last_time.tv_usec);

#else
	gettimeofday(&now, 0);		
	usec = now.tv_sec - last_time.tv_sec;
	usec = (usec * 1000000) + (now.tv_usec - last_time.tv_usec);
	datapackets++;
	if (usec > 1000000)
	{
		int z;

#endif
		sec = ((float) usec) / 1000000.;
		
		rate =  ((float) (totaldata - lasttotal)) / sec;
		
		info("\rTotal data received = %d Megs (%.3f Mbytes/sec); BUF :", (int) (totaldata >> 20), rate/ (1024. * 1024.));
#ifdef USE_DATA_BUFFER
		for (z=0; z < MAX_CONSUMERS;z++)
		if (data_buffer_consumer_exists(z))
		{
			info("%dM ", data_in_data_buffer_for(z) >> 20);
		}
		debug(" startpos = %d ", (int) (data_buffer_get_startpos() >> 20));
#else
		info("(none)");
#endif
		fflush(stderr);
		fflush(stdout);
		memcpy(&last_time, &now, sizeof(struct timeval));
		lasttotal = totaldata;

	}
	
}


/** End of data flow detected, do what must be done */
void process_end_of_data()
{
	/* flush the data buffer. -- will be done in main loop now
	 */
	info("End of data flow\n");
	flow_ended = 1;
	info("Flushing buffers\n");
	/* don't wait to send data anymore */
	send_thresold = 0;
}


#if 0
#ifdef USE_DATA_BUFFER	

	/* flush first the network consumers */
	for (z=0; z < ARITE_MAX;z++)
	if (children[z].exists)
	{
		
		while(data_in_data_buffer_for(children[z].consumer_id))
			send_from_data_buffer(children[z].consumer_id, SIZE_WHEN_SEND);
			
		
		drop_child(&children[z]);			
	}


	/* the flush the pipe */
	while(data_in_data_buffer_for(pipe_consumer_id))
			send_from_data_buffer(pipe_consumer_id, SIZE_WHEN_SEND);
				
		drop_data_buffer_for(pipe_consumer_id);
		close(pipe_write);
	

#else
	for (z=0; z < ARITE_MAX;z++) 
		if (children[z].exists)
		{
			close(children[z].fd);
			children[z].exists = 0;
		}
			
	close(pipe_write);
	pipe_write = -1;
#endif

#endif

void print_end_stats()
{
	struct timeval now;
	int usec;
	float sec;
	float rate;
	gettimeofday(&now, 0);
	info("Total data received = %d Megs, in %d packets\n", (int) (totaldata >> 20), datapackets);	
	
	usec = now.tv_sec - data_time.tv_sec;
	usec = (usec * 1000000) + (now.tv_usec - data_time.tv_usec);
	
	sec = ((float) usec) / 1000000.;
	rate =  ((float) totaldata) / sec;
	info("Elapsed time = %.3f seconds, throughput = %.3f Mbytes/second\n", sec, rate/ (1024. * 1024.));
		
	say_has_finished();
}
	

/** read incoming data, and call process_data() to store them in the 'data buffer' */
void take_incoming_data()
{
	int n;
	n = read(socket_data, read_buffer, BUFFERSIZE);
	
	/* read() returns 0 means no more data */
	if (n <= 0)
	{
		if (n == 0) 
			debug("Data connection (socket %d) closed on remote end\n", socket_data);
		else
			warn("Read error on data connection ! (socket %d) read : %s\n", socket_data, strerror(errno));

		say_dad_disconnected();
		close(socket_data);
		socket_data = -1;

	}
	else	
		process_data(read_buffer, n);
}
			
	
/** the main loop : block on select() and then read/write data to/from file descriptors */
void tcp_tree_main_loop()
{
	int i;
	int free_space;
	fd_set sockset_read;
	fd_set sockset_write;
	struct timeval select_delay;
	int select_ret;
	
	while((!children_finished) || (wait_others))
	{
		
		
		//memcpy((char *) &sockset_read, (char *) &sockset, sizeof(fd_set));		
		FD_ZERO(&sockset_read);
		FD_SET(socket_server, &sockset_read); 
		FD_SET(socket_command, &sockset_read);
#ifdef USE_DATA_BUFFER	
		FD_ZERO(&sockset_write);
		

		
		free_space = space_left_in_data_buffer();	

		/* if the data buffer gets full, don't accept data */
		if (socket_data >= 0)
			if (free_space >= WANT_FREE_SPACE) 
				FD_SET( socket_data, &sockset_read );
				
		/* see who are the children / pipes who can accept data */
		data_buffer_get_ready_consumers(&sockset_write, send_thresold);
		
		/* I also want to read on data connections, for error checking */
		for (i=0; i < ARITE_MAX; i++)
		{
			int fd;
			if (!children[i].exists) continue;
			fd = children[i].fd;
			/* hmm, I also want to know if there is sthg to read on those sockets -- it means ERROR ERROR*/
			FD_SET(fd, &sockset_read);
		}	
		
		select_delay.tv_usec = 0;
		select_delay.tv_sec = 50;

		select_ret = select(fd_max + 1, &sockset_read, &sockset_write, NULL, &select_delay);
#else
		select_ret =  select(fd_max + 1, &sockset_read, NULL, NULL, NULL);
#endif
		if (select_ret < 0)
		{
			perror("select");
#if 0
			/* some debug */
			for(i=0; i < fd_max +1; i++)
			{
				int read, write;
				read = FD_ISSET(i, &sockset_read);
				write = FD_ISSET(i, &sockset_write);
				if (read || write)
				{
					debug("fd %d : read = %s write = %s\n", i, read ? "yes" : "no" , write ? "yes" : "no");
					/* useless system call to see if this fd is OK */
					if (fcntl(i, F_GETFD) < 0)
						perror("fcntl");
				}
			}
#endif
			assert(0);
		}
			
		if ((select_ret == 0) && (socket_data > -1))
		{
			/* nothing happened last 5 second */
			/* send a keep-alive packet on the data connection :
			 * if the the daddy of this node fails, we may never see it if we send nothing on the socket_data
			 * so if we receive nothing during 5 second, we send a 4-byte packet on this socket, to check that the connection
			 * is still OK
			 **/
			int foo = 1234;
			debug("Sending keep-alive packet\n");
			if (write(socket_data, &foo, sizeof(int)) < 0)
			{
				warn("error sending keep alive packet : write : %s\n", strerror(errno));
				say_dad_disconnected();
				close(socket_data);
				socket_data = -1;
			}
		}

		if (socket_command >= 0)
		if (FD_ISSET(socket_command, &sockset_read))
		{
			int e = read_packet_from_server(socket_command);
			if (e < 0)
			{
				warn("Error with the server connection\n");
				abort();
			}
		}
		
		

		if (socket_server >= 0)		
		if (FD_ISSET(socket_server, &sockset_read))
		{
			accept_new_data_connection();
		}

		if (!flow_ended)						
		if (socket_data >= 0)
		if (FD_ISSET(socket_data, &sockset_read))
		{
			take_incoming_data();
		}
		
#ifdef USE_DATA_BUFFER			
		/* see if there is something to read on the children's data connections (there should be nothing) */
		for (i=0; i < ARITE_MAX; i++)
		{
			int fd;
			if (!children[i].exists) continue;
			fd = children[i].fd;
			if (FD_ISSET(fd, &sockset_read)) /* read keep-alive packets */
			{
				int foo;
				if (read(fd, &foo, sizeof(int)) <= 0)
				{
					debug("Error or connection closed on child data connection\n");
					drop_child(&children[i]); 
				}
			}
		}
		
		/* send the data to the pipe and the network clients */
		data_buffer_send_data_to_set(&sockset_write);		

		/* once we have finished reading data, try to see if we can
		 * 2/close the pipe/tcp connections with the children
		 * 1/exit
		 */
		if (!children_finished)
		if (flow_ended)
		{
		
			/* see if some children / the pipe have finished and drop them
			 * this is not very clean and should really be mixed with is_data_buffer_empty()
			 * to reduce CPU usage (not very critical)
			 * */
			/* drop children ? */
			int z;
			for (z=0; z < ARITE_MAX;z++)
				if (children[z].exists)
				{
					if (!data_in_data_buffer_for(children[z].consumer_id))
						drop_child(&children[z]);			
				}

			/* close the pipe ? */
			if (pipe_write > -1)
			if (!data_in_data_buffer_for(pipe_consumer_id))
			{
				debug("Closing pipe");
				drop_data_buffer_for(pipe_consumer_id);
				close(pipe_write);
				pipe_write = -1;
			}

			if (is_data_buffer_empty())
			{
				print_end_stats();
				children_finished = 1;
			}
		} /* flow_ended */

#endif		
		
	}
}


/** open a socket to listen on a TCP port */
int listen_port(unsigned short port)
{
	struct sockaddr_in sa;

	int sock;
	int e;
	int flag = 1;

	memset(&sa, 0, sizeof(struct sockaddr_in));

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port); 


	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) return -1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	

	e = bind(sock, (struct sockaddr *) &sa, sizeof(struct sockaddr_in));
	if (e < 0) 
	{
		close(sock);
		perror("bind");
		return -1;
	}

	listen(sock, 200); 

	debug("Socket %d on port %d ready.\n", sock, port);

	return sock;
}


/** Start the external command, its stdin being a pipe whose entry will be filedes 'pipe_write' */
void start_external_command()
{
	int pip[2];
	long orig;	
	command_started = 1;
	debug("Starting external command\n");
	
	assert(pipe(pip) >= 0);
	
	
	pipe_read = pip[0];
	pipe_write = pip[1];
	
	
	fd_max = max(pipe_read, fd_max);
	fd_max = max(pipe_write, fd_max);
	
	if (fork())
	{
		/* father process */
		close(pipe_read);
#ifdef USE_DATA_BUFFER
		pipe_consumer_id = add_data_buffer_consumer(pipe_write);
		orig = fcntl(pipe_write, F_GETFL);
 		fcntl(pipe_write, F_SETFL, orig | O_NONBLOCK);
		data_buffer_set_name(pipe_consumer_id, "pipe");
#endif
	}
	else
	{
		/* child process */
		int i;
		close(0);
		for (i=3; i <= fd_max; i++)
			if ((i!=pipe_read)) close(i); 
		
		assert(dup2(pipe_read, 0) >=0); /* duplicate fd to stdin */
		
		/* I use system rather than execv so I don't care about parameters parsing, PATH issues, etc. */
		exit(system(ext_command));
	}
}


int check_udp_reply(char * packet, int packet_len)
{
	char * ref = version;
	/*avoid buffer overflow */

	packet[packet_len - 1] = '\0'; 

	if (strlen(ref) + 1 != packet_len)
		return 0; /* false */

	if (strncmp(ref, packet, packet_len))
		return 0; /* false */
		
	debug("Version name matches\n");
	return 1;
}


int udp_find_server(char * session, IP * res_ip)
{
	struct hostent *he;
	int sock_listen;
	int retries = 10;
	struct timeval tv;

	fd_set sockset;
	
	char udppacket[UDP_MAX_PACKET_LEN];


	if ((he=gethostbyname("255.255.255.255")) == NULL) 
	{  /* get the broadcast address */
		perror("gethostbyname");
		exit(1);
	}

	FD_ZERO(&sockset);
	sock_listen = udp_listen(UDP_PORT_REPLY);
	FD_SET(sock_listen, &sockset);

	udp_send(*((struct in_addr *)he->h_addr), UDP_PORT_REQUEST, session, strlen(session) + 1);
	
	tv.tv_usec = 0;
	tv.tv_sec = 1;

	while (retries)
	{
		fd_set read_set;
		memcpy(&read_set, &sockset, sizeof(fd_set));
		
		if (select(sock_listen + 1, &read_set, 0, 0, &tv) > 0)
		{
			IP ip;
			int n;
			/* something happened */
			n = receive_udp_packet(sock_listen, udppacket, UDP_MAX_PACKET_LEN, &ip);
			if (n > 0)
			{
				debug("got packet from %s\n",inet_ntoa(ip));
				debug("packet is %d bytes long\n",n);
				if (check_udp_reply(udppacket, n))
				{
					close(sock_listen);
					*res_ip = ip;
					return 0;
				}
				else
					warn("Wrong server version\n");
				
			}

		}
		else
		{
			warn("Retrying...\n");
			udp_send(*((struct in_addr *)he->h_addr), UDP_PORT_REQUEST, session, strlen(session) + 1);
			retries--;
			tv.tv_usec = 0;
			tv.tv_sec = 1;

		}
	}

	return -1;

}


void usage()
{
#ifdef USE_RMTP_LIB
	printf("usage:ka-d-client [ -g ] [ -h host_name ] [ -s session_name ] [ -e command ] [ -m multicast adress ] [ -a ACK interval (multicast) ]\n");
#else
	printf("usage:ka-d-client [ -g ] [ -w ] [ -h host_name ] [ -s session_name ] [ -e command ]\n");
#endif
}

int main(int argc, char ** argv)
{
	int i;
	int only_go = 0;
	char ch;
#ifdef USE_RMTP_LIB
	char * optstr = "vh:a:s:gwe:m:";
#else
	char * optstr = "p:P:vh:s:gwe:m:";
#endif
	
	char * server_name;
	char * session_name;


	IP server_ip;

	assert(BUFFERSIZE + SIZE_TO_SEND + WANT_FREE_SPACE < DATA_BUFFER_SIZE);
	assert(WANT_FREE_SPACE > BUFFERSIZE);
	
	assert(argc);
	debug("Compiled : "__DATE__ " " __TIME__ "\n");
	
	

	server_name = session_name = 0;	
		
	while ( -1 != (ch = getopt(argc, argv, optstr)))
	{
		switch(ch)
		{
			case 'g': /* only go */
				only_go = 1;
				debug("Only say go to the server\n");				
				break;
				
			case 'w': /* only go */
				wait_others = 1;
				break;

			case 'p' : /* command tcp port num */
				tcp_port_comm = atoi (optarg);
				break;

			case 'P' : /* data tcp port num */
				tcp_port_data = atoi (optarg);
				break;

			case 'h' : /* give server host name */
				assert(server_name = strdup(optarg));
				break;
			
			
			case 'v' : /* version */
				printf("Ka-deploy client version %s\n", version);
				exit(0);
				break;

			case 'e' : /* give command */
				assert(ext_command = strdup(optarg));
				break;
#ifdef USE_RMTP_LIB						
			case 'm' : /* multicast */
				transport_mode = MULTICAST_TRANS;				
                                mcast.s_addr=inet_addr(optarg);
                                if (server_name) {
					printf ("Error : Multicast support will find the server itself, so do not use -s\n");
					exit(1);
				} else if (session_name) {
					printf ("Error : Multicast support does not rely on session names\n");
					exit(1);
				}
				break;
			case 'a' : /* ack interval for multicast */
				ack_interval = atoi(optarg);
				break;
#else
			case 's' : /* give session name */
				assert(session_name = strdup(optarg));
				break;
#endif /* ! USE_RMTP_LIB */				

			default:
				usage();
				exit(1);
				break;
		}
	}
		
		

	if (optind < argc) 
	{
		/* there are some arguments left on the command line, error */	
		usage();
		exit(1);
	}
			

	if ( (( (!server_name) && (!session_name) ) || ( (server_name) && (session_name) )) && !transport_mode)
	{
		usage();
		printf("Use either -s -h or -m\n");
		exit(1);
	}
		

	if (server_name && !transport_mode) 
		debug("Server is %s\n", server_name);	
		

		
	debug("command = %s\n", ext_command);
	
	
	if (!transport_mode) 
	{	
		if (!only_go)		
		{
#ifdef USE_DATA_BUFFER	
			init_data_buffer();
			debug("Data buffer of %d bytes allocated\n", DATA_BUFFER_SIZE);
#else
			warn("This program has been compiled without data buffer support\n");
#endif

			/* var init */
			for (i=0; i < ARITE_MAX;i++) children[i].exists = 0;


			/* start listen */
			socket_server = listen_port(tcp_port_data);
			assert(socket_server >=0);
			fd_max = max(socket_server, fd_max);
		}


		/* find server */
		if (session_name)
		{
			assert(udp_find_server(session_name, &server_ip) == 0);
		}
		else 
		{
			assert(get_ip_of(server_name, &server_ip) == 0);
		}


		info("Server IP is %s\n", inet_ntoa(server_ip));		
	
		/* call server */
	
		socket_command = call_port(server_ip, tcp_port_comm);
		if (socket_command < 0)
		{
			perror("connect");
			abort();
		}
		fd_max = max(socket_command, fd_max);	

	
		say_hello();
		send_options();
	}
	
	if (only_go)
	{
		say_go_now();
		return 0;
	}
	else 
	{
		int status;
		if (transport_mode == TCP_TREE_TRANS)
		{
			say_accept_data();
			tcp_tree_main_loop();
		}
#ifdef USE_RMTP_LIB
		else if (transport_mode == MULTICAST_TRANS) /* multicast ! I take control here :) */
		{
			multicast_main_loop();

		}
#endif		
		debug("Waiting for child to terminate...\n");		
		wait(&status);
		debug("Child exited with status %d\n", status);		
	}
	return 0;
}
