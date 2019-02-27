/*
 *
 * $Revision: 1.37 $
 * $Author: sderr $
 * $Date: 2002/06/13 13:55:03 $
 * $Header: /cvsroot/ka-tools/ka-deploy/src/server.c,v 1.37 2002/06/13 13:55:03 sderr Exp $
 * $Id: server.c,v 1.37 2002/06/13 13:55:03 sderr Exp $
 * $Name:  $
 * $Log: server.c,v $
 * Revision 1.37  2002/06/13 13:55:03  sderr
 * Removed the uid == 0 check. It belongs to ka-d.sh now.
 *
 * Revision 1.36  2002/06/07 08:19:56  sderr
 * Did the same end-of-flow buffer flushing as in the client [in main select()]
 *
 * Revision 1.35  2002/06/07 07:58:50  sderr
 * Improved end-of-buffer flushing in the client [done in the main select() loop now]
 *
 * Revision 1.34  2002/06/06 14:16:22  sderr
 * changed usec variable to long long to avoid negative times
 *
 * Revision 1.33  2002/05/31 08:52:37  sderr
 * TCP ports can now be changed with -p and -P options
 *
 * Revision 1.32  2002/05/23 09:14:19  sderr
 * Mostly doc and script updates
 *
 * Revision 1.31  2002/03/18 14:16:24  sderr
 * Fixed non-existant-child bug when a client fails before tree construction
 *
 * Revision 1.30  2002/03/15 09:53:00  sderr
 * Fixed nasty bug (missing fflush())
 *
 * Revision 1.29  2001/12/21 15:13:03  sderr
 * Fixed case when a waiting client (with the -d option on the server)
 * exits unexpectedly (or has no -w)
 *
 * Revision 1.28  2001/12/17 11:28:01  sderr
 * Added a -d option in ka-d-server to add a delay between the clients when we release them
 *
 * Revision 1.27  2001/12/14 13:10:29  sderr
 * Some fixes. The server-waits-last-client can cause perf pb with many nodes,
 * so now it is the -l option
 * Fixed problematic -w in the install script
 *
 * Revision 1.26  2001/11/19 13:39:43  sderr
 * Quite many changes, because I wanted to improve the client death detection
 * This has been harder than expected, because now the _server_ can be the one who des the detection
 * --> more client/server exchanges
 * --> after a data connection is opened, auth step now includes a report_position from the child
 *
 * Revision 1.25  2001/11/16 11:32:34  sderr
 * -Clients now send keep-alive packets to their daddy when they are inactive
 *
 * Revision 1.24  2001/11/15 16:44:18  sderr
 * -minor cleanups
 * -fixed TWO silly bugs around my old 'unsigned long long' -> introduced offset_t type, see in buffer.h
 *
 * Revision 1.23  2001/11/14 15:41:18  sderr
 * Now the last client of the chain tells the server from time to time
 * what data it has, and so the server
 * can make sure he will have the necessary data in case of failure
 * (that is all the report_pos) stuff
 * -still only for the chain
 * -if we lose the last client, transfer stops (should be easy to fix)
 *
 * Revision 1.22  2001/11/14 10:34:57  sderr
 * Many many changes.
 * -silly authentication done by client/server communication on data connections
 * -it was needed for this : recovery when one client fails. It takes a lot of changes
 * This seems to work, but :
 * 	* error detection : well, it's ok when I kill -INT a client, that's all
 * 	* sometimes the node re-contacted does not have the data the re-contacting node needs -- needs to be fixed
 * 	* works only in the arity = 1 case.
 *
 * Revision 1.21  2001/11/09 14:37:37  sderr
 * Major changes on the server, which uses also the data buffer now.
 * Cleanups on the server, the filedes are -at last- not used anymore as array indexes (i.e. much crap removed)
 * I think I can expect a performance boost, but also a higher CPU utilization.
 *
 * Revision 1.20  2001/11/09 10:15:31  sderr
 * Some cleanups, mostly in client.c
 * Moved the data_buffer stuff to buffer.c and buffer.h so I'll be able to use it on the server also
 * Added the new struct consumer
 *
 * Revision 1.19  2001/09/13 14:59:15  sderr
 * Added an option (-w) which says to the clients that they have to wait
 * until all clients have finished the transfer before exiting.
 *
 * Revision 1.18  2001/08/17 14:20:52  mikmak
 * performance improvement for multicast support and updated usage/help
 *
 * Revision 1.17  2001/07/02 11:39:13  sderr
 * Moved things so that:
 * 	-multicast support is optional (check Makefile)
 * 	-.c files again can be compiled using gcc, not g++ (added multicast_server.cpp and multicast_client.cpp)
 * Cleaned transport_mode variable
 *
 * Revision 1.16  2001/07/01 15:27:18  mikmak
 * multicast is now integrated and working. Many updates to rmtplib and optimization for Ka.Still much to do ...
 *
 * Revision 1.15  2001/06/29 09:31:45  sderr
 * *** empty log message ***
 *
 * Revision 1.14  2001/06/06 09:52:02  sderr
 * Added comments
 *
 * Revision 1.13  2001/06/05 08:51:57  sderr
 * Added [ -e command ] command line option
 *
 * Revision 1.12  2001/06/01 09:34:07  sderr
 * Added comments, removed useless nb_client variable
 *
 * Revision 1.11  2001/06/01 09:04:26  sderr
 * The tree now gets built only when all the clients have contacted the server, so that the tree can be adapted to some network topology.
 * Implemented minimal adaptation method : sort IP addresses in ascendent order.
 *
 * Revision 1.10  2001/05/30 14:35:51  sderr
 * Added udp support, clients now can find the server by sending UDP broadcasts
 *
 * Revision 1.9  2001/05/30 13:16:09  sderr
 * Added getopt use for command-line parsing
 *
 * Revision 1.8  2001/05/21 10:03:13  sderr
 *
 * Revision 1.7  2001/05/21 09:19:14  sderr
 *
 * Revision 1.6  2001/05/11 09:42:40  sderr
 * Added some time measures
 *
 * Revision 1.5  2001/05/10 12:57:01  sderr
 * Clients can no more connect when the transfer has started.
 * Added must_start_transfer and transfer_started to replace
 * the wanted_clients and pipe_read modifications side effects.
 *
 * Revision 1.4  2001/05/10 12:18:02  sderr
 * Added comments, fixed open comment on CVS header
 * Function renaming in client
 *
 * Revision 1.3  2001/05/03 12:26:24  sderr
 * Just playing with CVS.. again :)
 *
 * $State: Exp $
 * $Sources$
 */

#include "packet.h"
#include "tree.h"
#include "udp.h"
#include "buffer.h"
#include "delay.h"


#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#include "server.h"

#define SORT_IP
/** if SORT_IP is defined, the clients will be added in the tree with their IPs in ascendent order,
 this can help to minimize inter-switch communications if arity = 1*/

static char version[] = SOFT_VERSION;

#define EXTCOMMAND "(cd /; tar  --create --one-file-system --sparse /)"

// COMMANDS BELOW ARE FOR DEBUGGING PURPOSES :
//#define EXTCOMMAND "(cd /; tar cl /home)"
//#define EXTCOMMAND "(cd /usr/X11R6; tar cl /usr/X11R6)"
//#define EXTCOMMAND "(cd /usr; tar cl /usr)"
//#define EXTCOMMAND "dd if=/dev/zero bs=1000k count=1000"
//#define EXTCOMMAND "ping -c 10 zuni"
//#define EXTCOMMAND "(cd /home; tar cl /home)"


char * ext_command = EXTCOMMAND;



/** arity of the tree */
int arite;

/** number of clients we want before we start the transfer */
int wanted_clients;

int busy_clients = 0;

/** name of the session used for UDP-broadcast-based server finding */
char * session_name;


unsigned short tcp_port_comm = PORTNUMCOMM;
unsigned short tcp_port_data = PORTNUMDATA;

#ifdef USE_RMTP_LIB
/* Multicast IP */
IP mcast;
int win_size=0;

/* from multicast_server.cpp : */
extern  void multicast_main_loop(); 

#endif /* use multicast */

enum { TCP_TREE_TRANS, MULTICAST_TRANS } transport_modes;

/* Transport mode */
int transport_mode = TCP_TREE_TRANS; 


/** must_start_transfer : this var is used to know where we are in the process of accepting the clients
	- waiting_for_clients : we still want more clients to contact us
	- must_build_tree : we have enough clients, or a clients said 'go', we will build the tree on the next main loop iteration
	- building_tree : we have sent each client its peer IP, we are waiting for them to report the connections 
	- tree_built : all inter-clients connections have been made
	- tree_in_use : transfer has started
	
	this var was previously a boolean, and maybe should get another name.
*/
enum must_start_transfer_state { waiting_for_clients, must_build_tree, building_tree, tree_built, tree_in_use };
int must_start_transfer = waiting_for_clients;
	


/** delay_clients_exit : option: be sure to introduce a delay between the release time of the clients */
int delay_clients_exit = 0;

/** wait_last_client : option, to ensure data recovery always sucessfull */
int wait_last_client = 0;

/** connected_clients : the number of clients who have established a data connection with their peer (daddy) */
int connected_clients = 0;

/** clients_wanting_data : the number of clients who said they want data */
int clients_wanting_data = 0;

/** transfer_started : boolean : transfer has started ? */
int transfer_started = 0;

/** clients_ping_running : boolean : 0 --> we must send ping packets; 1 --> we must count replies */
int clients_ping_running = 0;

/** flow_ended : boolean : has the server read all the data ? */
int flow_ended = 0;

/** children_finished : boolean : has the server sent all the data ? */
int children_finished = 0;

/** min data in the buffer to call write() */
unsigned int send_thresold = SIZE_TO_SEND;


/** use a struct client to represent the server at the top of the tree */
struct client * myself;

/** struct connection : data structure associated to each control connection */
enum conn_state { no_conn, connected, got_hello, wants_data };

struct connection
{
	enum conn_state state;
	IP peer_ip;
	int fd;
	struct client * client;
};

/** control connections opened by the clients */
#define MAX_CONN 300
struct connection connections[MAX_CONN];

/** clients who have successfully said 'hello' and who accept data*/
#define MAX_CLIENTS 300
struct client *  clients_pool[MAX_CLIENTS];

int nb_clients_in_pool = 0;

/** server IP */
IP my_ip;

/** socket for the control connection listener */
int socket_server = -1;

/** max file descriptor number, used with select() call */
int fd_max = -1;

/** fd_set used with select() */
fd_set sockset;

/** socket for listening to UDP requests */
int udp_sock = -1;

/** socket for the data connection listener */
int socket_data_server = -1;

/** a child is someone who has a data connection with us */
struct child
{
	int exists;
	int fd;
	int consumer_id; // for the data buffer	
	IP ip;
	int auth;
};

struct child children[ARITE_MAX];
int nb_children = 0;

/** the pipe with the external command */
int pipe_read = -1; /** server end of the pipe */
int pipe_write = -1; /** external command end of the pipe */


/* two different max() functions -- I'm not too fond of macros for this one
 */
int max(int a, int b) { return a > b ? a : b; }
offset_t  max_off(offset_t a, offset_t b) { return a > b ? a : b; }


/*
 * amount of data we have read from the data pipe
 */
offset_t totaldata = 0;

/*
 * amount of data received reported by the last client last time he did so
 * -- and also a struct client * on this client --
 */
offset_t last_client_pos = 0;
struct client * last_client = 0;

/** get the IP of this machine.
	fills the IP pointed by ip parameter
	returns 0 if all is OK
	returns -1 if a problem occured
*/
int get_my_ip(IP * ip)
{
	char myhostname[MAXHOSTNAME + 1];

	struct hostent * he;


	/* I suppose there is an easier way of getting my IP, but ... */
	/* have a look to RMTPclient.cc function get_ip(char *device), it allows you to choose your net interface, this could be a command line option
	 * which you could pass to RMTPlib (setInterface()) but it is certainly not _easier_ than your function
	 * */ 

	gethostname(myhostname, MAXHOSTNAME);

	he = gethostbyname(myhostname);
	if (he == NULL) return -1;
	memcpy((char *) ip, he->h_addr, he->h_length);
 
	return 0;
}	

/* IPv4, 32bit only HACK !!!!! */
/** same_ip : returns true if a and b are the same IP address */
int same_ip(IP a, IP b)
{
	int *x = (int *) &a;
	int *y = (int *) &b;
	return *x == *y;
}

/* IPv4, 32bit only, UGLY HACK !!!!! */
/** compare_ip : comparison between two IP addresses used to sort them */
int compare_ip(IP a, IP b)
{
	int *x = (int *) &a;
	int *y = (int *) &b;
	return *x - *y;
}


/** listen_port : open a TCP socket to listen on a specified port;
	returns -1 if it fails, socket number otherwise
*/
int listen_port(unsigned short port)
{

	char myhostname[MAXHOSTNAME + 1];

	struct sockaddr_in sa;
	struct hostent * he;

	int sock;
	int e;
	int flag = 1;

	memset(&sa, 0, sizeof(struct sockaddr_in));

	gethostname(myhostname, MAXHOSTNAME);

	he = gethostbyname(myhostname);
	if (he == NULL) return -1;

	sa.sin_family = he->h_addrtype;
	sa.sin_port = htons(port); 


	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) return -1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	

	e = bind(sock, (struct sockaddr *) &sa, sizeof(struct sockaddr_in));
	if (e < 0) 
	{
		close(sock);
		return -1;
	}

	listen(sock, 200); 

	debug("Socket %d on port %d on %s ready.\n", sock, port, myhostname);

	return sock;
}








/** send_packet_client : send a packet to a client over a control connection
*/
int send_packet_client(struct client * client, struct server_packet * packet)
{
	int e;
	struct connection * conn = client->conn;
	assert(conn);
	assert(conn->state >= got_hello);
	
	e = write(client->conn->fd, packet, sizeof(struct server_packet));
	if (e < 0) 
	{
		warn("While writing on socket %d : write : %s\n", client->conn->fd, strerror(errno));
	}
	return e;
}

/** give_ip_client : when building the tree, tell to a client which IP he has to connect to
*/
int give_ip_client(struct client * client, IP ip)
{
	struct server_packet packet;
	struct packet_server_gives_ip * pdata;
	
	packet.type = server_gives_ip;
	pdata = (struct packet_server_gives_ip *) &(packet.data);
	pdata->ip = ip;
	
	return send_packet_client(client, &packet);
	
}

/** send_auth : when a client connected to another, tell the daddy it is the right client
*/
int send_auth(struct client * dad, struct client * son)
{
	struct server_packet packet;
	struct packet_server_auth_child * pdata;
	
	packet.type = server_gives_auth;
	pdata = (struct packet_server_auth_child *) &(packet.data);
	pdata->ip = son->ip;
	pdata->data_offset = son->data_offset;	
	debug("sending auth for %s to ", inet_ntoa(son->ip));
	debug("%s\n", inet_ntoa(dad->ip));
	return send_packet_client(dad, &packet);

}

/** say_drop_child : tell a client to drop one of his (failing) children
*/
int say_drop_child(struct client * dad, struct client * son)
{
	struct server_packet packet;
	struct packet_server_says_drop_child * pdata;
	
	packet.type = server_says_drop_child;
	pdata = (struct packet_server_says_drop_child *) &(packet.data);
	pdata->ip = son->ip;
	return send_packet_client(dad, &packet);
}

/** say_finish_client : sends a 'packet_server_says_finish' to a client : sent when one clients is waiting for the others to exit
*/
int say_finish_client(struct client * client)
{
	struct server_packet packet;
	
	packet.type = server_says_finish;
	
	return send_packet_client(client, &packet);	
}


/** say_end_of_flow : sends a 'end' to a client : 
 *  when a client gets disconnected by his daddy, and that it is normal (end of flow)
*/
int say_end_of_flow(struct client * client)
{
	struct server_packet packet;

	packet.type = server_says_end_of_flow;
	
	return send_packet_client(client, &packet);	
}

/** send a ping packet to a client
*/
int send_ping(struct client * client)
{
	struct server_packet packet;

	packet.type = server_sends_ping;
	
	return send_packet_client(client, &packet);	
}

/** ask a client to report its position in the data flow.
 * yes_or_no = no -> never
 * yes_or_no = yes -> regularly
 * yes_or_no = once -> once
*/
int say_please_report_position(struct client * client, int yes_or_no)
{
	struct server_packet packet;
	struct packet_server_says_please_report_position * pdata;

	packet.type = server_says_please_report_position;
	pdata = (struct packet_server_says_please_report_position *) &(packet.data);
	pdata->yes_or_no = yes_or_no;
	
	return send_packet_client(client, &packet);	
}



/** drop one of our children
*/
void drop_child(struct child * c)
{
	close(c->fd);
	drop_data_buffer_for(c->consumer_id);
	c->exists = 0;
	warn("Dropping child %s\n", inet_ntoa(c->ip));
}


/** find a struct child with the given IP address
 **/
struct child * find_child(IP cip)
{
	int i;
	for (i=0; i < ARITE_MAX; i++)
		if (children[i].exists) debug("Child %d : %s\n", i, inet_ntoa(children[i].ip));

	for (i=0; i < ARITE_MAX; i++)
		if ((children[i].exists) && (same_ip(children[i].ip, cip))) return &children[i];

	return 0;
}



/** add a client * in the tree, somewhere under the given 'root' 
  return value : true if levels_under of this root has been changed */
int add_client_in_tree_under(struct client * root, struct client * leaf)
{
	if (root->nb_children < arite)
	{
		/* ok, stop here */
		root->children[root->nb_children++] = leaf;
		leaf->daddy = root;
		if (root->nb_children == arite) root->levels_under = 1;
		return (root->nb_children == arite); /* depth changed only if this is our last child */
	}
	else
	{
		/* find amongst the children which has the lowest 'under' */
		int num = 0;
		int min = 9999999;
		int i;
		int changed;
		for (i=0; i < arite; i++)
		{
			if (root->children[i]->levels_under < min)
			{
				min = root->children[i]->levels_under;
				num = i;
			}
		}

		changed = add_client_in_tree_under(root->children[num], leaf);

		if (changed)
		{
			int oldval = root->levels_under;
			min = 9999999;
			for (i=0; i < arite; i++)
			{
				if (root->children[i]->levels_under < min)
				{
					min = root->children[i]->levels_under;
					num = i;
				}
			}
			min++;

			if (min > oldval)
			{
				root->levels_under = min;
				return 1;
			}
			else
				return 0;
		}
	}
	// NOT REACHED
	return 0;
}
				
/** add_client_in_tree : add a client in the tree
 */
void add_client_in_tree(struct client * leaf)
{
	char buf[20];
	add_client_in_tree_under(myself, leaf);
	
	strcpy(buf, inet_ntoa(leaf->ip));
	debug("Added client %s, daddy = %s\n", buf, inet_ntoa(leaf->daddy->ip));
}

/** new_client : creates a new struct_client and initializes it
*/
struct client * new_client(IP ip, struct connection * conn)
{
	struct client * p = (struct client *)malloc(sizeof(struct client));
	
	assert(p);
	
	p->ip = ip;
	p->conn = conn;
	p->nb_children = 0;
	p->levels_under = 0;
	p->data_connected = 0;
	p->busy = 0;
	p->auth_done = 0;
	p->dropped = 0;
	p->is_waiting_release = 0;
	/* initialize options */
	p->will_wait_for_others = 0;
	p->dad_wants_position = 0;
	p->data_offset = 0;
	return p;
}	


/** tree_client_got_client : when a client reports that one of his children opened the data connection between them,
	update our data structures. 
	Broken function name, sorry
	--first packet : data connection opened
	--second packet : auth ACK

	UPDATE: now returns a struct client * on the child
	--> used when the server himself needs auth.
*/
struct client * tree_client_got_client(struct client * daddy, IP child_ip)
{
	int i;
	for (i=0; i < daddy->nb_children; i++)
	{
		struct client * son = daddy->children[i];
		// y'a un truc qui manque la comme.. son->exists ou chais pas quoi.. ah peut-etre pas en fait
		if (same_ip(son->ip,child_ip))
		{
			if (son->auth_done)
			{
				assert(!son->data_connected);
				son->data_connected = 1;
				connected_clients++;
				// if this is a reconnection -- the client can be busy while not connected
				if (!son->busy) 
				{
					son->busy = 1;
					busy_clients++;
				}
				debug("%s reports ", inet_ntoa(daddy->ip));
				debug("%s has been accepted\n", inet_ntoa(child_ip));
			}
			else
			{
				debug("%s reports ", inet_ntoa(daddy->ip));
				debug("%s has opened data connection\n", inet_ntoa(child_ip));
				son->dad_wants_position = 1;
				say_please_report_position(son, rp_once);
			}
			return son;
		}
	}
	/* child has not been found, bug hanging around -- OR someone else (kaxx0r :) tried to connect */
	printf ("ERROR CHILD NOT FOUND\n");
	printf ("daddy->ip = %s\n", inet_ntoa(daddy->ip));
	printf ("conn IP = %s\n", inet_ntoa(child_ip));

	assert(0);
}
		

/** draw_tree_under : print the structure of the tree under a given client.
	recursive function
*/
void draw_tree_under(struct client * root, int tabs)
{
	int i;
	/* indentation */
	if (arite > 1)
	for (i=0; i < tabs; i++) putchar('\t');

	
	debug("IP=%s, fd=%d, nb_children=%d, depth=%d (%s)\n", inet_ntoa(root->ip), root->conn ? root->conn->fd : -1, root->nb_children, root->levels_under, root->data_connected ? "connected" : "NOT connected");
	

	for (i=0; i < root->nb_children; i++)
		draw_tree_under(root->children[i], tabs+1);
}


/** add_client_to_pool : add a client to the pool of clients 
*/
void add_client_to_pool(struct client * cli)
{
	int i;
	assert(nb_clients_in_pool < MAX_CLIENTS);

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients_pool[i] == 0)
		{
			clients_pool[i] = cli;
			nb_clients_in_pool++;
			return;
		}
	warn("Not enough room in pool for new client (?!?)\n");
	assert(0);
}


/**  
*/
void init_pool()
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		clients_pool[i] = 0;
		
	
	nb_clients_in_pool = 0;
			
}


/**  
*/
void remove_from_pool(struct client * cli)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients_pool[i] == cli)
		{
			clients_pool[i] = 0;
			nb_clients_in_pool--;
			return;
		}
}


/** qsort_helper : the function I use with qsort() when I create the tree from the pool 
*/
int qsort_helper(const void * a, const void * b)
{
	struct client * client_a;
	struct client * client_b;	
	int r;
	client_a = *((struct client **) a);
	client_b = *((struct client **) b);
	
	r = compare_ip(client_a->ip, client_b->ip);
	return r;
	
}

/** build_tree_from_pool : build the tree with the clients from the pool, trying to take into account the 'topology' of the network.
	For my network I only have to sort the IP addresses, in the case where arity = 1
*/
void build_tree_from_pool()
{
	
	int i;
	struct client * cli = 0;
	
#ifdef SORT_IP
	qsort(clients_pool, nb_clients_in_pool, sizeof(struct client *), qsort_helper);
#endif

	for (i = 0; i < nb_clients_in_pool; i++)
	{
		cli = clients_pool[i];
		
		
		add_client_in_tree(cli);

		/* give to the client the IP of its future peer */
		give_ip_client(cli, cli->daddy->ip); 
	}

	if (wait_last_client)
	{
		// ask the las client to report its position
		assert(cli);
		say_please_report_position(cli, rp_yes);
		last_client = cli;
	}
}




/** show_nbclients : just print the nimber of connected clients 
*/
void show_nb_clients()
{
	debug("Clients : want_data %d  / connected %d\n", clients_wanting_data, connected_clients);
}


/** draw_tree : print the whole tree
*/
void draw_tree()
{
	draw_tree_under(myself, 0);
	show_nb_clients();
}

/*
 * update the data structures and send information to other clients when a node fails
 * -- beware -- : this function will be called only ONCE for each failure, 
 * and therefore must do ALL the work completely
 */
	
void tree_drop_client(struct client * client) /* called BY close_connection */
{
		struct client * dad = 0;
		struct client * son = 0;

		int i;

		assert(arite == 1);
		
		assert(client != myself); // someone has lost data connection with ME -- wtf ???
		
		son = client->children[0];
		dad = client->daddy;
		assert(dad);


		// tell the grand father that he must drop the 'father' (his child)
		if (dad != myself)
			say_drop_child(dad, client);
		else
		{
			// the failing node is one of OUR (our = the server) children
			struct child * victim = find_child(client->ip);
			// maybe we have already detected his failure and dropped him
			if (victim) drop_child(victim);
		}
	
		warn("Client FAILURE : %s\n", inet_ntoa(client->ip));

		if (son)
		{
			son->auth_done = 0;	
			/* once i got a false assertion here.. i even don't know HOW it could happend
			 * It happened after I stopped a client with crtl-c
			 */
			assert(son->data_connected);
			son->data_connected = 0;
			connected_clients--;

			/* hmmm.. here I try all the children, above I take children[0]...some lack of consistency maybe : fixme */
			for (i=0; i < arite; i++)
				if (dad->children[i] == client)
				{
					dad->children[i] = son;
					son->daddy = dad;
					give_ip_client(son, dad->ip);
				}
		}
		else
		{
			if (wait_last_client)
			{
				/* now the dad is the end of the chain : please report */
				say_please_report_position(dad, rp_yes);
				last_client = dad;
			}
		}

}
				

void announce_client_has_finished(struct client * client);

/** close_connection : when we close a connection, we might have data structures to update.
	completely broken function
*/
int close_connection(struct connection * conn)
{
	if (conn->state == no_conn) return 0;
	debug("close_connection(%d)\n", conn->fd);
	if (conn->state == wants_data)
	{
		struct client * client = conn->client;
		assert(client);
		assert(!client->dropped);
		/* when we arrive here after the transfer has finished, clients are data_connected but NOT busy
		 * A busy client means failure
		 */
		if ((client->data_connected) && (client->busy)) tree_drop_client(client);

		if (client->data_connected) connected_clients--;
		if (client->busy) 
		{
			busy_clients--;
			if (busy_clients == 0)
				announce_client_has_finished(client);
		}

		
		if (!transfer_started)
			remove_from_pool(client);
	
		clients_wanting_data--;

		client->dropped = 1; // don't do anything with this client then.


		if (client->is_waiting_release)
			wipe_from_wait_queue(client);
	}
	
	conn->state = no_conn;
	close(conn->fd);
	FD_CLR(conn->fd, &sockset);		
	debug("Busy clients: %d -- connected : %d\n", busy_clients, connected_clients);
	return -1;
}

int drop_client(struct client * client)
{
	return close_connection(client->conn);
}

int add_new_connection()
{
	int i;
	for (i = 0; i < MAX_CONN; i++)
		if (connections[i].state == no_conn)
		{
			return i;
		}
		
	return -1;
}

		
/** accept_new_connection : a clients wants to open a control connection with us, welcome him
*/
void accept_new_connection()
{
	struct sockaddr_in client_addr;
	socklen_t client_addr_size;
	int socket_client;
	IP ip;
	int conn_id;

/*	printf("Accepting new connection\n"); */
	
	memset(&client_addr,0, sizeof(client_addr));
	client_addr_size = sizeof(client_addr);
	socket_client = accept(socket_server, (struct sockaddr *) &client_addr, &client_addr_size);
	ip = client_addr.sin_addr;
	
	debug("Accepting connection from %s\n", inet_ntoa(ip));
	
	if (socket_client >= MAX_CONN)
	{
		warn("uh, oh, connection with fd = %d\n", socket_client);
		close(socket_client);
		return;
	}


	conn_id = add_new_connection();
	assert(conn_id >= 0);
	
	if (socket_client > fd_max) fd_max = socket_client;
	FD_SET(socket_client, &sockset);	

	connections[conn_id].state = connected;
	connections[conn_id].fd = socket_client;
	connections[conn_id].peer_ip = ip;
	connections[conn_id].client = 0; /* may be useful for some assertions */

	show_nb_clients();
}


/** THE FOLLOWING FUNCTIONS CONTAIN PROCESSING TO BE DONE WHEN RECEIVING COMMANDS ON A CONTROL CONNECTION.
	They are named according to this scheme : int process _packet_name
*/


/** process_client_says_hello : means process(client_says_hello) : a client sent a 'hello' packet, do what must be done.
	this means : check software version, create data structure for this client
*/
int process_client_says_hello(struct packet_client_says_hello * packet, struct connection * conn)
{
	
	if (conn->state != connected) /* no conn, or already got hello */
	{
		close_connection(conn);
		return -1;
	}
	
	packet->version[VERSION_LEN-1]='\0';
	if (strcmp(packet->version, version))
	{
		close_connection(conn);
		return -1;
	}
	
	/* everything is fine */
	conn->state = got_hello;
	conn->client = new_client(conn->peer_ip, conn);

	
	debug("client says hello !\n");
	return 0;	
}
		
/** process_client_got_client : one of the clients has established a connection with his daddy 
*/
int process_client_got_client(struct packet_client_got_client * packet, struct connection * conn)
{
	// find the struct client
	struct client * client = conn->client;
	assert(client);
	debug("Client got client\n");

	tree_client_got_client(client, packet->child_ip);
	return 0;
}

/** process_client_sends_options : 
*/
int process_client_sends_options(struct packet_client_sends_options * packet, struct connection * conn)
{
	// find the struct client
	struct client * client = conn->client;
	assert(client);
	debug("Client sends options\n");

	client->will_wait_for_others = packet->will_wait_for_others;
	return 0;
}

/** process_client_accepts_data : a client says he accepts data
*/
int process_client_accepts_data(struct packet_client_accepts_data * packet, struct connection * conn)
{
	// find the struct client
	struct client * client = conn->client;
	assert(client);

	if (conn->state != got_hello) 
	{
		close_connection(conn);
		return -1 ;
	}
	conn->state = wants_data;
	debug("Client accepts data\n");
	
	clients_wanting_data++;
	
/*	add_client_in_tree(conn->client); */

	
	/* give to the client the IP of its future peer */
/*	return give_ip_client(conn->client, conn->client->daddy->ip); */
	
	add_client_to_pool(conn->client);
	
	return 0;
	
}


void really_accept_new_child(IP cip, offset_t data_offset);

/** process_client_reports_position : a client says he accepts data
*/
int process_client_reports_position(struct packet_client_reports_position * packet, struct connection * conn)
{
	// find the struct client
	struct client * client = conn->client;
	assert(client);
	if (client->dad_wants_position)
	{
		debug("Client %s reports data position : %lld\n", inet_ntoa(client->ip), packet->data_received);
		client->dad_wants_position = 0;
		client->auth_done = 1;
		client->data_offset = packet->data_received;
		if (client->daddy != myself)
			send_auth(client->daddy, client);
		else
			really_accept_new_child(client->ip, packet->data_received);
	}

	if (wait_last_client)
	{
		assert(last_client);
		if (client == last_client)
			last_client_pos = packet->data_received;
	}

	return 0;
	
}

/** process_client_says_go_now : a client wants the transfer to start RIGHT NOW
*/
int process_client_says_go_now(struct packet_client_says_go_now * packet, struct connection * conn)
{
	// find the struct client
	struct client * client = conn->client;
	assert(client);

	if (conn->state != got_hello) 
	{
		close_connection(conn);
		return -1 ;
	}

	debug("Client wants the data transfer to start NOW\n");
	must_start_transfer = must_build_tree;  /* will be read in next main_loop iteration */
	close_connection(conn);
	return 0;
}


/** FFFFFFFFIIIIIIIIIXXXXXXXXXXXMMMMMMMMMEEEEEEEEE
*/
int process_client_has_finished(struct packet_client_has_finished * packet, struct connection * conn)
{
	// find the struct client
	struct client * client = conn->client;
	assert(client);
	debug("Client says he has finished\n");


	debug("Client has finished transfer\n");
	if (client->busy)
	{
		client->busy = 0;
		busy_clients--;
		debug("Busy clients: %d -- connected : %d\n", busy_clients, connected_clients);
	}

	announce_client_has_finished(client);

	return 0;
}


/* REALLY bad name. sometimes we don't announce anything */
void announce_client_has_finished(struct client * client)
{

	if (delay_clients_exit)
	{
		if (client->will_wait_for_others) add_to_wait_queue(client);
	}
	else
	{
		if (busy_clients == 0) {
			int i;
			for (i = 0; i < nb_clients_in_pool; i++)
			{
				struct client * cli = clients_pool[i];
				if (!cli->dropped) if (cli->will_wait_for_others) say_finish_client(cli);
			}
		}
	}			
}

int process_client_says_dad_disconnected(struct packet_client_says_dad_disconnected * packet, struct connection * conn)
{
	debug("Client says dad disconnected\n");
	assert(conn->client);

	if (!same_ip(conn->client->daddy->ip, packet->dad_ip))
	{
		debug("Old news\n");
		return 0;
	}


	if ((flow_ended) && (packet->data_received == totaldata))
		say_end_of_flow(conn->client);
	else
	{
		warn("Client lost %s lost his data connection ", inet_ntoa(conn->peer_ip));
		debug("Has received %lld out of %lld bytes\n", packet->data_received, totaldata);

		assert(arite == 1);
		
		assert(conn->client->daddy != myself); // client has lost data connection with ME -- wtf ???
		
		drop_client(conn->client->daddy);
	}

		

	return 0;
}

int process_client_ping_reply(struct connection * conn)
{
	assert(conn->client);
	conn->client->answered_ping = 1;
	return 0;
}

/** process_packet : got a packet on a control connection, process it
*/
int process_packet(struct client_packet * packet, struct connection * conn)
{
//	printf("Got packet from client %s, type = %d\n", inet_ntoa(conn->peer_ip), packet->type);
	switch(packet->type)
	{
		case client_says_hello:
			return process_client_says_hello((struct packet_client_says_hello *) &(packet->data), conn);
		case client_got_client:
			return process_client_got_client(&(packet->data.got_client), conn);
		case client_says_has_finished:
			return process_client_has_finished(&(packet->data.has_finished), conn);
		case client_accepts_data:
			return process_client_accepts_data(&(packet->data.accepts_data), conn);
		case client_says_go_now:
			return process_client_says_go_now(&(packet->data.go_now), conn);
		case client_sends_options:
			return process_client_sends_options(&(packet->data.options), conn);
		case client_reports_position:
			return process_client_reports_position(&(packet->data.report_pos), conn);
		case client_says_dad_disconnected:
			return process_client_says_dad_disconnected(&(packet->data.disconn), conn);
		case client_ping_reply:
			return process_client_ping_reply(conn);
			
	}
	return -1;
}

/** read_packet_from_socket : select() told us there is something available on a control connection, read it
*/
int read_packet_from_socket(struct connection * conn)
{
	struct client_packet packet;
	int n;
	int socket = conn->fd;
	
/*	printf("Reading from socket %d\n", socket); */
	n = read(socket, (void *) &packet, sizeof(struct client_packet));
	
	/* don't bother, my packets are small and should be read in exactly one try */
	if (n!= sizeof(struct client_packet))
	{
		if (n>0) warn("Wrong packet size on socket %d\n", socket);
		if (n==0) warn("Peer closed connection on socket %d\n", socket);
		if (n<0) warn("Error on socket %d : %s\n", socket, strerror(errno));
		close_connection(conn);
		return -1;
	}
		
	return process_packet(&packet, conn);
}


/** accept_new_child : One of our children is establishing a data connection with us, accept it
*/
void accept_new_child()
{
	struct sockaddr_in client_addr;
	socklen_t client_addr_size;
	int socket_client;
	struct child * son;
	struct client * child_client;
	IP ip;
	//int flag = 1;
	long orig;

	memset(&client_addr,0, sizeof(client_addr));
	client_addr_size = sizeof(client_addr);
	socket_client = accept(socket_data_server, (struct sockaddr *) &client_addr, &client_addr_size);
	ip = client_addr.sin_addr;
	
	debug("Accepting connection from %s\n", inet_ntoa(ip));
	
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
	
	debug("checking connection auth");
	child_client = tree_client_got_client(myself, ip);	
	if (!child_client) 
	{
		warn("unauthorized connection -- closing\n");
		close(socket_client);
		return;
	}

	
	if (socket_client > fd_max) fd_max = socket_client;
	
	son = &children[nb_children++];
	son->exists = 1;
	son->fd = socket_client;
	son->auth = 0;
	// as a matter of fact the auth is OK, but i'm waiting for a report_position packet before I really consider so
	son->ip = ip;
	
	orig = fcntl(socket_client, F_GETFL);
 	fcntl(socket_client, F_SETFL, orig | O_NONBLOCK);
}


void really_accept_new_child(IP cip, offset_t data_offset)
{
	struct child * son = 0;
	int err;
	int i;

	for (i = 0; i < nb_children; i++)
		if (same_ip(children[i].ip, cip)) son = &children[i];

	assert(son);

	// ACK the auth
	tree_client_got_client(myself, cip);	
	debug("Welcome son, you are number %d (MAX %d)\n", nb_children, ARITE_MAX);
	son->consumer_id = add_data_buffer_consumer(son->fd);
	assert(son->consumer_id >= 0);
	err = data_buffer_set_consumer_pos(son->consumer_id, data_offset);
	if (err < 0)
	{
		warn("Error : client wants data i do not have !\n");
		abort();
	}
}


#define BUFFERSIZE 32768
static char read_buffer[BUFFERSIZE];

int datapackets = 0;


struct timeval start_time;
struct timeval end_flow_time;
struct timeval quit_time;


/** transmit_data : we know there are data available from the external command on the pipe,
	read them and do smthg with it
*/
void transmit_data()
{
	int n;

	

	n = read(pipe_read, read_buffer, BUFFERSIZE);
	
	assert(n >= 0);
	if (n == 0)
	{
		debug("\nEnd of data flow\n");
		gettimeofday(&end_flow_time, 0);
		flow_ended = 1;
		debug("Dropping children\n");
		FD_CLR(pipe_read, &sockset);				
		close(pipe_read);
		pipe_read = -1;
		send_thresold = 0;
		return;
	}
	
	/* put data in the buffer - they will be sent later - after the next select() - maybe very soon, */	
	store_in_data_buffer(read_buffer, n);

	totaldata += n;
	if ((datapackets++ & 0xff) == 0) 
	{
		int z;
		// for each 256 packets, print status
		info("\rTotal data read = %d Megs, BUF: ", (int) (totaldata >> 20));
		for (z=0; z < MAX_CONSUMERS;z++)
		if (data_buffer_consumer_exists(z))
		{
			info("%dM ", data_in_data_buffer_for(z) >> 20);
		}
		debug(" FREE = %dM ", space_left_in_data_buffer() >> 20);
		debug(" startpos = %dM", (int) (data_buffer_get_startpos() >> 20));
		if (wait_last_client) { debug(" lastclientpos = %dM", (int) (last_client_pos >> 20)); }
		fflush(stderr);
	}
			
}

/** print_statistics : print misc infos about transfer times and throuput at the end of the data transfer
*/
void print_statistics()
{
	long long usec;
	float sec;
	float rate;

	info("Total data sent = %d Megs, in %d packets\n", (int) (totaldata >> 20), datapackets);	
	
	usec = end_flow_time.tv_sec - start_time.tv_sec;
	usec = (usec * 1000000) + (end_flow_time.tv_usec - start_time.tv_usec);
	
	sec = ((float) usec) / 1000000.;
	rate =  ((float) totaldata) / sec;
	info("Transfer time = %.3f seconds, throughput = %.3f Mbytes/second\n", sec, rate/ (1024. * 1024.));

	usec = quit_time.tv_sec - end_flow_time.tv_sec;
	usec = (usec * 1000000) + (quit_time.tv_usec - end_flow_time.tv_usec);
	
	sec = ((float) usec) / 1000000.;

	if (!delay_clients_exit) info("The pipeline was emptied in %.3f seconds\n", sec);
}

/** start_external_command : start the external command :
	open pipes
	fork
	close pipes that must be closed
	exec command
*/
	
void start_external_command()
{
	int pip[2];
	

	debug("Let's go!\n");
	
	assert(pipe(pip) >= 0);
	
	
	pipe_read = pip[0];
	pipe_write = pip[1];
	
	
	fd_max = max(pipe_read, fd_max);
	fd_max = max(pipe_write, fd_max);
	
	if (fork())
	{
		// father process
		close(pipe_write);
		FD_SET(pipe_read, &sockset);
		gettimeofday(&start_time, 0);
	}
	else
	{
		// child process
		int i;
		fflush(stdout); // MUHAHAHAHA NASTY
		for (i=0; i <= fd_max; i++)
			if (i!=pipe_write) close(i);
		
		assert(dup2(pipe_write, 1) >=0); // duplicate fd to stdout
		
		system(ext_command);
		
		exit(0);
	}
}
	

/** send_udp_reply : reply to an UDP request we have received
*/
void send_udp_reply(IP dest)
{
	/* reply by sending the version, can be useful */
	char * rep = version;
	debug("Sending UDP reply to %s\n", inet_ntoa(dest));
	udp_send(dest, UDP_PORT_REPLY, rep, strlen(rep) + 1);	
	
}

/** process_udp_packet : see what must be done after we received an UDP request
*/
void process_udp_packet(IP source, char * packet, int packet_len)
{
	/** avoid buffer overflow */
	packet[packet_len - 1] = '\0'; 

	if (strlen(session_name) + 1 != packet_len)
		return;

	if (strncmp(session_name, packet, packet_len))
		return;
		
	debug("Session name matches\n");
	send_udp_reply(source);
}

/** main_loop :
	do always
		select()
		read on file descriptors and do what has to be done
	loop
*/
	
void tcp_tree_main_loop()
{
	int i;
	int select_ret;
	fd_set fdsr;
	fd_set sockset_write;
	int free_space;
	struct timeval select_delay;

	for(;;)
	{
		if (!transfer_started) 
		{
			if ((clients_wanting_data >= wanted_clients) && (must_start_transfer == waiting_for_clients)) 
				must_start_transfer = must_build_tree;
			/* must_start_transfer can also be set in process_client_says_go_now() */
			
			if ((clients_wanting_data == connected_clients) && (must_start_transfer == building_tree)) 
				must_start_transfer = tree_built;			
			
			
			
			if (must_start_transfer == must_build_tree) 
			{
				build_tree_from_pool();
				must_start_transfer = building_tree;
			}
				
			
			if (must_start_transfer == tree_built)
			{
				must_start_transfer = tree_in_use;
				
				start_external_command();
				transfer_started = 1;
				/* we don't accept connections anymore */
				if (udp_sock != -1)
				{
					close(udp_sock); 
					FD_CLR(udp_sock, &sockset);
					udp_sock = -1;	
				}
				close(socket_server); 
				FD_CLR(socket_server, &sockset);	socket_server = -1;	
				/* NO NO NO : if we want data recovery -- we do not close this listening socket : 
				close(socket_data_server); 
				FD_CLR(socket_data_server, &sockset); socket_data_server = -1;
				*/
				
			}
		}
				
		if ((children_finished) && (connected_clients == 0))				
		{
			gettimeofday(&quit_time, 0);
			info("All clients left, I quit\n");
			print_statistics();
			return;
		}

		


		memcpy((char *) &fdsr, (char *) &sockset, sizeof(fd_set));
		FD_ZERO(&sockset_write);
		
		free_space = space_left_in_data_buffer();	

		/* if the data buffer gets full, don't accept data */
		/* OR if the last client is too far -- for recovery */
		if (pipe_read >= 0)
			if ((free_space < WANT_FREE_SPACE) || 
					((wait_last_client) &&
					(!still_in_data_buffer(max_off(last_client_pos - BUFFERSIZE, 0))))
				)
				FD_CLR( pipe_read, &fdsr );

		//if (pipe_read >= 0)
		//	if (free_space < WANT_FREE_SPACE) 
		//		FD_CLR( pipe_read, &fdsr );
				
		/* see who are the children / pipes who can accept data */
		data_buffer_get_ready_consumers(&sockset_write, send_thresold);
		
		/* I also want to read on data connections, for error checking */
		for (i=0; i < ARITE_MAX; i++)
		{
			int fd;
			if (!children[i].exists) continue;
			fd = children[i].fd;
			/* hmm, I also want to know if there is sthg to read on those sockets -- it means ERROR ERROR*/
			FD_SET(fd, &fdsr);
		}	


		/* if the flow has not ended, loong select delay for detecting congestion (failed client)
		 * if the flow has ended, short delay for a not too unaccurate client release delay
		 */
		if (children_finished)
		{
			select_delay.tv_usec = 100000;
			select_delay.tv_sec = 0;
		}
		else
		{
			select_delay.tv_usec = 0;
			select_delay.tv_sec = 5;
		}

		select_ret = select(fd_max + 1, &fdsr, &sockset_write, NULL, &select_delay);
		//printf("s = %d\n", select_ret);	
		if (select_ret < 0)
		{
			perror("select");
#if 0
			/* some debug */
			for(i=0; i < fd_max +1; i++)
			{
				int read, write;
				read = FD_ISSET(i, &fdsr);
				write = FD_ISSET(i, &sockset_write);
				if (read || write)
				{
					printf("fd %d : read = %s write = %s\n", i, read ? "yes" : "no" , write ? "yes" : "no");
					/* useless system call to see if this fd is OK */
					if (fcntl(i, F_GETFD) < 0)
						perror("fcntl");
				}
			}
#endif
			sleep(1);
			continue;
		}


		/* try to release clients who have finished */
		if (children_finished)
		{
			struct client * cli;
			while ((cli = try_to_release_client())) say_finish_client(cli);
		}
		
	
		if (transfer_started) 
		if (!children_finished)
		if (select_ret == 0)
		{
			/* nothing happened in 5 seconds ? I'm almost sure one client failed 
			 * check them
			 * 
			 * I know this method of using select() timeout for a client ping is a bit... lazy
			 * but is should be appropriate for us
			 */
			if (clients_ping_running)
			{
				/* we are already running a client check -- this is the time limit
				 * clients wo did not answer MUST DIE DIE DIE 
				 **/
				debug("Checking ping replies\n");
				for (i = 0; i < nb_clients_in_pool; i++)
				{
					struct client * cli = clients_pool[i];
					if (!cli->dropped)
					if (!cli->answered_ping)
						drop_client(cli);
				}
				clients_ping_running = 0;
				
			}
			else
			{
				/* no client ping running, start one then
				 */
				clients_ping_running = 1;
				warn("I'm stuck, sending pings to the clients\n");	
				for (i = 0; i < nb_clients_in_pool; i++)
				{
					struct client * cli = clients_pool[i];
					if (!cli->dropped) 
					{
						cli->answered_ping = 0;
						send_ping(cli);
					}
				}
			}
			continue;
		}
				

		if (udp_sock >= 0)
		if (FD_ISSET(udp_sock, &fdsr))
		{
			char udppacket[UDP_MAX_PACKET_LEN];
			int n;
			IP ip;
			n = receive_udp_packet(udp_sock, udppacket, UDP_MAX_PACKET_LEN, &ip);
			if (n > 0)
			{
				debug("got UDP packet from %s\n",inet_ntoa(ip));				
				process_udp_packet(ip, udppacket, n);
			}
		}


		if (socket_server >= 0)
		if (FD_ISSET(socket_server, &fdsr))
		{
			accept_new_connection();
		}

		if (socket_data_server >= 0)
		if (FD_ISSET(socket_data_server, &fdsr))
		{
			accept_new_child();
			// draw_tree();			
		}
		
		if (pipe_read >= 0)
		if (FD_ISSET(pipe_read, &fdsr))
		{
			transmit_data();
		}

		/* send the data to the network clients */
		data_buffer_send_data_to_set(&sockset_write);

		for (i=0; i <= MAX_CONN; i++)
			if (connections[i].state != no_conn)
				if (FD_ISSET(connections[i].fd, &fdsr))
					read_packet_from_socket(&connections[i]);
		
		for (i=0; i < ARITE_MAX; i++)
		{
			int fd;
			if (!children[i].exists) continue;
			fd = children[i].fd;
			if (FD_ISSET(fd, &fdsr)) /* read keep-alive packets */
			{
				int foo;
				if (read(fd, &foo, sizeof(int)) <= 0)
					drop_child(&children[i]); // uh-oh
			}
		}
		

		/* once we have finished reading data, try to see if we can
		 * 2/close tcp connections with the children
		 * 1/exit
		 */
		if (!children_finished)
		if (flow_ended)
		{
		
			/* see if some children have finished and drop them
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

			if (is_data_buffer_empty())
			{
				children_finished = 1;
				debug("All children dropped\n");
			}
		} /* flow_ended */

		
		/* when the transfer has not begun, I print the tree structure */
		// if (!transfer_started) if (!flow_ended) draw_tree();
	}
}
			
/** usage : print command line syntax
*/
void usage()
{
#ifdef USE_RMTP_LIB
	printf("usage:ka-d-server [ -n nb_clients ] [ -e command ] [ -m multicast address ] [ -w Maximum Send Window size (multicast) ]\n");
#else
	printf("usage:ka-d-server [ -l ] [ -n nb_clients ] [ -a arity ] [ -s session_name ] [ -d delay ] [ -e command ]\n");
#endif
}

/** main :
	-check command_line
	-init data structures
	-open server sockets
	-enter main loop
*/
int main(int argc, char ** argv)
{
	int i;
	
	
	char ch;
#ifdef USE_RMTP_LIB
	char * optstr = "vn:e:m:A:w:";
#else
	char * optstr = "p:P:vd:la:s:n:e:";
#endif

	
	debug("Compiled : "__DATE__ " " __TIME__ "\n");
	
	debug("ARGS=");
	for (i=0; i < argc; i++)
		debug("+%s", argv[i]);
	debug("+\n");
	
	arite = 1;
	wanted_clients = 1;
	session_name = 0;
	
	
	while ( -1 != (ch = getopt(argc, argv, optstr)))
	{
		switch(ch)
		{
			case 'n' : /* number of clients expected */
				wanted_clients = atoi(optarg);
				break;
				
			case 'd' : /* client release delay */
				delay_clients_exit = 1;
				set_wait_delay(atoi(optarg) * 100000);
				break;

			case 'p' : /* command tcp port num */
				tcp_port_comm = atoi (optarg);
				break;

			case 'P' : /* data tcp port num */
				tcp_port_data = atoi (optarg);
				break;

			case 'l' : /* wait last client */
				wait_last_client = 1;
				break;
			case 'v' : /* version */
				printf("Ka-deploy server version %s\n", version);
				exit(0);
				break;
				
				
			case 'e' : /* give command */
				assert(ext_command = strdup(optarg));
				break;

#ifdef USE_RMTP_LIB			
			case 'm' : /* multicast address */
				transport_mode = MULTICAST_TRANS;
				mcast.s_addr=inet_addr(optarg);
				if (!arite) {
					printf ("Error : Multicast support and arity are not compatible, you can choose to use the TCP pipe (arity) or multicast\n");
					exit(1);
				}else if (session_name) {
					printf ("Error : Multicast support does not rely on session names\n");
					exit(1);
				}				
				break;
				
			case 'w' : /* Maximum send window size */
				win_size = atoi(optarg);
				break;
#else
			case 's' : /* give session name */
				assert(session_name = strdup(optarg));
				break;
			
			case 'a': /* define arity */
				arite = atoi(optarg);
				break;
#endif
				
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
			
	
	get_my_ip(&my_ip);
	debug("Server IP = %s\n", inet_ntoa(my_ip));
	debug("command = %s\n", ext_command);
	
	
	debug("I want %d clients\n", wanted_clients);
	FD_ZERO(&sockset);
	for (i=0; i < ARITE_MAX;i++) children[i].exists = 0;
	
	
	if (session_name && (transport_mode == TCP_TREE_TRANS))
	{
		udp_sock = udp_listen(UDP_PORT_REQUEST);
		assert(udp_sock >=0);
		FD_SET(udp_sock, &sockset);
		fd_max = max(fd_max, udp_sock);
	}
	
	
	if (transport_mode == TCP_TREE_TRANS) { /* listen for clients only if i'm not using multicast, rmtplib handles clients connection itself */
		socket_server = listen_port(tcp_port_comm);
		assert(socket_server >=0);
		FD_SET(socket_server, &sockset);
		fd_max = max(fd_max, socket_server);
	
		socket_data_server = listen_port(tcp_port_data);
		assert(socket_data_server >=0);
		FD_SET(socket_data_server, &sockset);
		fd_max = max(fd_max, socket_data_server);
		
		init_data_buffer();
	}
	
	myself = new_client(my_ip, 0);
	myself->data_connected = 1; /* looks nicer with draw_tree() */
	
	for (i=0; i < MAX_CONN; i++)
	{
		connections[i].state = no_conn;
	}

	init_pool();

#ifdef USE_RMTP_LIB
	if (transport_mode == MULTICAST_TRANS)
		multicast_main_loop();
	else
#endif /* USE_RMTP_LIB */
		tcp_tree_main_loop();

	return 0;
}





















