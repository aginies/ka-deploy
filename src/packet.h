

/*
 * $Revision: 1.12 $
 * $Author: sderr $
 * $Date: 2002/05/23 09:14:19 $
 * $Header: /cvsroot/ka-tools/ka-deploy/src/packet.h,v 1.12 2002/05/23 09:14:19 sderr Exp $
 * $Id: packet.h,v 1.12 2002/05/23 09:14:19 sderr Exp $
 * $Log: packet.h,v $
 * Revision 1.12  2002/05/23 09:14:19  sderr
 * Mostly doc and script updates
 *
 * Revision 1.11  2001/11/19 13:39:43  sderr
 * Quite many changes, because I wanted to improve the client death detection
 * This has been harder than expected, because now the _server_ can be the one who des the detection
 * --> more client/server exchanges
 * --> after a data connection is opened, auth step now includes a report_position from the child
 *
 * Revision 1.10  2001/11/15 16:44:18  sderr
 * -minor cleanups
 * -fixed TWO silly bugs around my old 'unsigned long long' -> introduced offset_t type, see in buffer.h
 *
 * Revision 1.9  2001/11/14 15:41:18  sderr
 * Now the last client of the chain tells the server from time to time
 * what data it has, and so the server
 * can make sure he will have the necessary data in case of failure
 * (that is all the report_pos) stuff
 * -still only for the chain
 * -if we lose the last client, transfer stops (should be easy to fix)
 *
 * Revision 1.8  2001/11/14 10:34:57  sderr
 * Many many changes.
 * -silly authentication done by client/server communication on data connections
 * -it was needed for this : recovery when one client fails. It takes a lot of changes
 * This seems to work, but :
 * 	* error detection : well, it's ok when I kill -INT a client, that's all
 * 	* sometimes the node re-contacted does not have the data the re-contacting node needs -- needs to be fixed
 * 	* works only in the arity = 1 case.
 *
 * Revision 1.7  2001/11/09 10:15:31  sderr
 * Some cleanups, mostly in client.c
 * Moved the data_buffer stuff to buffer.c and buffer.h so I'll be able to use it on the server also
 * Added the new struct consumer
 *
 * Revision 1.6  2001/10/10 13:55:05  sderr
 * Updates in documentation
 *
 * Revision 1.5  2001/09/13 14:59:15  sderr
 * Added an option (-w) which says to the clients that they have to wait
 * until all clients have finished the transfer before exiting.
 *
 * Revision 1.4  2001/07/01 15:27:18  mikmak
 * multicast is now integrated and working. Many updates to rmtplib and optimization for Ka.Still much to do ...
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



#ifndef PACKET_H_INCLUDED
#define PACKET_H_INCLUDED


#include <netinet/in.h>
#include "buffer.h" // for offset_t

/* this is only the protocol version */
#define SOFT_VERSION "Ka-deploy-0.6"
#define VERSION_LEN 25

#define MAXHOSTNAME 200
#define PORTNUMCOMM 30765
#define PORTNUMDATA 30764


/** client-server protocol (not complete)
 *
 * server			client1			client2
 * 		<-- hello
 * 		<--------------------------- hello
 * 		------> giveIP(server)
 * 		---------------------------------> giveIP(client1)
 * 		<------ open data connection
 * 					<----- open data connection
 * 		<-- client_got_client(client2)
 * 		-----------------------------> please_report_pos(once)
 * 		<------------------------------------- report_pos()
 * 		-------> send_auth(client2, pos)
 *
 * 		-- data -------->>----------------------->>
 *
 * 		when data has been transferred, close(data_connections)
 *
 * 		<--------- dad_disconnected(pos)
 * 		<---------------------------------- dad_disconnected(pos)
 * 		------------> end_of_flow()
 * 		-------------------------------------> end_of_flow()
 * 				exit
 * 							exit
 * 	exit
 *
 * this is a simplified (not mentioned options, client waiting for others...) case
 * of a transfer when everything works OK
 */


typedef struct in_addr IP;

/* packet sent by the server to a client :
the server gives to the client the IP of its pseudo-server */
struct packet_server_gives_ip
{
	IP ip;
};

struct packet_server_says_drop_child
{
	IP ip;
};

/* the server confirms that the data connection a client got is OK */
struct packet_server_auth_child
{
	IP ip;
	offset_t data_offset;
};
/* packet sent by the server to a client :
GO ! */
struct packet_server_says_go
{
	int foo;
};

struct packet_server_says_finish
{
};

enum rp_val { rp_no, rp_yes, rp_once };

struct packet_server_says_please_report_position
{
	enum rp_val yes_or_no;
};


enum server_packet_type 
{ 
	server_says_go, 
	server_says_end_of_flow,
	server_says_finish,
	server_gives_ip,
	server_gives_auth,
	server_says_drop_child,
	server_says_please_report_position,
	server_sends_ping
};


struct server_packet
{
	enum server_packet_type type;
	union 
	{
		struct packet_server_gives_ip ip;
		struct packet_server_says_go go;
		struct packet_server_auth_child auth;
		struct packet_server_says_drop_child drop;
		struct packet_server_says_please_report_position report_pos;
	} data;
};


/* this is what a client sends to any client server it connects to */
struct packet_client_says_hello
{
	char version[VERSION_LEN];
};

struct packet_client_accepts_data
{
};

struct packet_client_says_go_now
{
};

struct packet_client_sends_options
{
	unsigned char will_wait_for_others;
};

struct packet_client_has_finished
{
};

struct packet_client_got_client
{
	IP child_ip;
};

struct packet_client_says_dad_disconnected
{
	IP dad_ip;
	offset_t data_received;
};

struct packet_client_reports_position
{
	offset_t data_received;
};




enum client_packet_type
{
	client_says_hello,
	client_got_client,
	client_accepts_data,
	client_says_dad_disconnected,
	client_says_has_finished,
	client_reports_position,
	client_says_go_now,
	client_sends_options,
	client_ping_reply
};

struct client_packet
{
	enum client_packet_type type;
	union 
	{
		struct packet_client_says_hello hello;
		struct packet_client_got_client got_client;
		struct packet_client_accepts_data accepts_data;
		struct packet_client_has_finished has_finished;		
		struct packet_client_says_go_now go_now;	
		struct packet_client_says_dad_disconnected disconn;	
		struct packet_client_sends_options options;
		struct packet_client_reports_position report_pos;
	} data;
};


#endif /* PACKET_H_INCLUDED */
