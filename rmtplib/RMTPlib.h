/*
 * $Revision: 1.3 $
 * $Author: mikmak $
 * $Date: 2001/07/01 15:27:18 $
 * $Header: /cvsroot/ka-tools/ka-deploy/rmtplib/RMTPlib.h,v 1.3 2001/07/01 15:27:18 mikmak Exp $
 * $Id: RMTPlib.h,v 1.3 2001/07/01 15:27:18 mikmak Exp $
 * $Log: RMTPlib.h,v $
 * Revision 1.3  2001/07/01 15:27:18  mikmak
 * multicast is now integrated and working. Many updates to rmtplib and optimization for Ka.Still much to do ...
 *
 * Revision 1.2  2001/06/29 13:48:35  mikmak
 * modifying variables names
 *
 * Revision 1.1  2001/06/29 12:04:58  mikmak
 * added
 *
 * $State: Exp $
 */

#ifndef RMTP_H
#define RMTP_H

// CONFIGURABLE PART
// Explanation : WINDOW_SIZE and ACK_INTERVAL are certainly the most important,
// first ACK_INTERVAL should never be bigger than WINDOW_SIZE this configuration won't work at all
// after testing, using a ACK_INTERVAL which is about 2/3 of WINDOW_SIZE is a good thing and should be used
// this is more important when you have a high number of clients
// for small number of clients you can use smaller value as it will increase RMTPserver's speed in emission
// RMTPlib also provides a way to use a tree architecture for getting back ACKs, it works but should be used only for
// high number of clients more than 2 hundreds i guess. Sadly, this is not fully working yet and does not give all the
// expected possibilities

#define RMTP_MAX_CLIENTS	250
#define WINDOW_SIZE	45 // send window size, recv window is dynamic			
#define PACKET_SIZE	1420 // this should be the maximal value on ethernet (MTU:1500), PACKET_SIZE is the data section size of the packet
#define INTERFACE	"eth0"

//these are default values and can be changed using the setParam() function
//change the values here if you dont want to use the setParam for each program, used to fit your specific network needs for example
#define COMMAND_PORT 3004
#define DATA_PORT 3005
#define MAX_CLIENTS_LEVEL RMTP_MAX_CLIENTS	
#define MAX_RELAYS_LEVEL 0 
#define ACK_INTERVAL 34	
#define TRANSMIT_TIMEOUT 1 // in seconds, as we consider to be on a fast network i will certainly change this to ms soon
#define TTL 32 

//END OF CONFIGURABLE PART

// DO NOT CHANGE ANYTHING AFTER THIS !
// 
//

#define DEBUG 
#define STATS 

// these are the different packet types i use 
#define GET_MASTER 10 // new client is joining multicast group
#define IAMMASTER 14 // master gives its IP to client
#define CLIENT_NORMAL 11 // client is _normal_
#define CLIENT_RELAY 12 // client is a relay
#define SERVER 13 // just for commodity
#define DATA_PACKET 20 // all data packets
#define DATA_EOF 25 // last packet of flow 
#define RETRANSMIT 30 // a client ask for a retransmission
#define RETRANSMITTED 35 // packet retransmitted
#define ACK 40 // a client gives the list of packets successfully received
#define CLIENT_READY 50 // a client is ready to receive
#define CLIENT_LEAVE 60 // a client has disconnected
#define DIE 70 // the server asks a client to leave now, for ex when a client has lost too many packets which could mean a network problem
#define ASK_ACK 80 // the server asks the client to send its ACKs now since we're waiting for him !
#define NEW_CHILD 90 // a relay has a new child
#define REQ_SYNC 100 // master or relay is asking a sync , occurs when the desynchronisation between clients is too big
#define EOS 110 // end of sync for a client

/*******************************
 * setParam() parameters defines
 * 
 ******************************/
// ->client only
#define SET_ACK_INTERVAL 1001

// ->server only
#define SET_MAX_CLIENTS_LEVEL 1002
#define SET_MAX_RELAYS_LEVEL 1003
#define SET_TIMEOUT 1007

// ->both
#define SET_COMMAND_PORT 1005
#define SET_DATA_PORT 1006
#define SET_TTL 1008

/****************************
 * status defines for getStatus()
 *
 ****************************/
#define RMTP_INIT 10500;
#define RMTP_TRANSFERING 10510
#define RMTP_TRANSFER_SUCCESS 10520

/****************************
 * Error codes
 * 
 ***************************/
#define CLIENT_CONNECTED -2001 // this occurs when setParam (SET_MAX_*) is called after a client has connected
#define INIT_END -2002 //this occurs when setParam (SET_*_PORT) is called whereas the init has terminated, init is considered closed when a transfer has started
#define NO_CLIENT -2003
#define INVALID_PARAMETER -2004
#define WRONG_INTERFACE -2005
#define WRONG_TTL -2006



#include <netinet/in.h>
#include <pthread.h>
#include <sys/time.h>
#include "RMTPlist.h"

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

class RMTPServer;
class RMTPClient;

typedef struct in_addr IP;

typedef struct PacketCommand
{
	int type;
	unsigned long seq;
	IP src;
};


typedef struct PacketData
{
	int type; // type of the packet , see before
	unsigned long seq; // the unique sequence number
	IP src; // source IP used for retransmission in unicast
	int len; // the length of the data
	int crc; // checksum
	char buff[PACKET_SIZE]; // datas
};

typedef struct PacketList
{
	PacketData pkt;
	struct list_head l;
	PacketList() {
		INIT_LIST_HEAD(&this->l);
	}
	bool status;
	
};

typedef struct Client
{
	IP ip_client;
	IP parent;
	bool receive_ready;
	int id;
	int type;
	int nb_child;
	int level;
};

class RMTPServer
{
	public:
		RMTPServer(IP *multicast);
		// returns the number of clients connected
		int getNbClients();
		// this function can only be used before a transfer has started, you'll get an error otherwise
		// SET_ACK_INTERVAL, SET_COMMAND_PORT, SET_DATA_PORT, SET_TIMEOUT, SET_TTL are common to server and client, 
		// this requires you to call setParam in your server _and_ in your client otherwise you'll probably get nothing
		// SET_MAX_CLIENTS_LEVEL, SET_MAX_RELAYS_LEVEL are server side only, trying to access them in RMTPClient will return you an error
		int setParam(int param,int value);
		// select the network interface to use
		int setInterface(char *iface);
		// clear everything, close the sockets and discards clients
		void shutdown();
		// reopen sockets, automatically calls shutdown() if necessary
		void restart();
		// use this function to send a specific buffer to a given number of clients, call is blocking
		// if no send occurs before t_out send RMTPsend returns an error
		int RMTPsend(char *buf,int len, int clients=0);
		// should be used for file transfer, starts when 'clients_connected' are connected or immediately if 0
		int startTransfer(int source, int clients_connected=0);
		// check whether we are : RMTP_INIT, RMTP_TRANSFERING, RMTP_TRANSFER_SUCCESS
		int getStatus();
		~RMTPServer();

	private:
		int readSource(char *dest, int qty);
		void reset();
		void processPacket(PacketCommand *pack);
		void sendPacketCommand(PacketCommand *pack);
		void sendPacketData(PacketData *pack);
		void transmit();
		void fillQueue();
		void fillGaps();
		void newClient(PacketCommand *pack);
		void retransmit(PacketCommand *pack);
		void getAck(PacketCommand *pack);
		void clientReady(PacketCommand *pack);
		friend void start_server(RMTPServer *serv);
		void clientLeaving(PacketCommand *pack);
		void initQueue();
		void listQueue();
		void listACKs();
		void sendRetrans(PacketData *pack,IP *ip);
		void erasePacksInQueue();
		void say(IP *dest, IP *info,int mess);
		void printStats();
		void buildTree(IP *client);
		void deleteClientFromTree(IP *ip);
		int initSockets(int portdata,int portcommand,unsigned char ttl);
		bool isrelay(IP *ip);
		void askSync(unsigned long seq);
		void getSync(PacketCommand *pack);
		void checkSync();
		void statusWindow();
		
		int server_state;
		int server_status;
		int transfer_mode;
		char *source_buf;
		int source_len;
		int wait_for_client;
		bool syncing;
		unsigned long syncseq;
		unsigned long list_sync[RMTP_MAX_CLIENTS];
		pthread_t listener;
		Client liste_client[RMTP_MAX_CLIENTS]; // this one contains MY clients
		Client tree[RMTP_MAX_CLIENTS+1]; // this one contains all the tree
		int fd_command,fd_data,fd_source;
		bool sending; 
		bool reading;
		bool init;
		PacketData window_send[WINDOW_SIZE];
		bool swindow_status[WINDOW_SIZE]; // indicates whether a packet has been sent or not
		unsigned int ack_list[RMTP_MAX_CLIENTS];
		unsigned long seq_index;
		unsigned long last_seq_send;
		unsigned long totaldata;
		unsigned long totalretrans;
		unsigned long nb_packs_retrans;
		IP my_ip;
		IP mcast;
		int nb_clients,nb_clients_total;
		struct timeval start_time;
		struct timeval end_flow_time;
		time_t last_send;
		bool wait_discard;
		bool no_client;
		int nb_level;
		int command_port,data_port;
		int timeout;
		int nb_max_clients;
		int nb_max_relays;
		unsigned char ttl;
		bool client_connected;
		char interface[6];
		
	protected:
};

class RMTPClient
{
	public:
		RMTPClient(IP *multicast);
		//first connect to multicast server
		void connect();
		//close the connection
		void disconnect();
		//set specific parameters for the session, must be done before receive() and RMTPrecv()
		int setParam(int param,int value);
		//set the interface to use (eth0 is default)
		int setInterface(char *iface);
		//start reception and send datas to fd_out
		void receive(int fd_out);
		// use this to receive data sent with RMTPsend(), if server sends more data than len then it will be lost 
		// setting len to a high value is a good idea: it will stop receiving when the server stops sending 
		// even if it received less than len bytes (your buffer has to be big enough)
		int RMTPrecv(char *buf,int len);
		int getStatus();
		~RMTPClient();

	private:
		int transfer_mode;
		char *dest_buff;
		int dest_len;
		void sayReady();
		void reset();
		int client_status;
		IP my_ip;
		IP mcast;
		IP parent;
		IP master;
		pthread_t listener;
		unsigned long nb_packet_received; 
		int fd_command,fd_data,fd_dest,wait;
		unsigned long last_seq_completed; // last seq sent to fd_dest
		bool receiving;
		void say(int what,IP *to);
		void transmitToDest();
		void retransmit(long seq);
		void fillQueue();
		void fillGaps();
		void listQueue();
		void initQueue();
		void processPacket(PacketData *pack);
		void processPacket(PacketCommand *pack); 
		void sendAck();
		void statusWindow();
		friend void start_client(RMTPClient *client);
		friend void start_relay(RMTPClient *client);
		bool relay;
		Client liste_client[RMTP_MAX_CLIENTS];
		unsigned int ack_list[RMTP_MAX_CLIENTS];
		bool no_client;
		int nb_clients;
		bool syncing;
		unsigned long list_sync[RMTP_MAX_CLIENTS];
		void getSync(PacketCommand *pack);
		void sendSync();
		void checkSync();
		void askSync(unsigned long seq);
		unsigned long syncseq;
		int command_port,data_port;

		int writeDest(char *src, int qty);
		void erasePacksInQueue();
		void getAck(PacketCommand *pack);
		void newClient(PacketCommand *pack);
		void clientLeaving(PacketCommand *pack);
		void resend(PacketCommand *pack);
		void sendRetrans(PacketData *pack,IP *ip);
		void clientReady(PacketCommand *pack);
		int initSockets(int dataport,int commandport, unsigned char ttl);
		int findMin();
		int findMax();
		bool isfull();
		int timeout;
		int nb_max_clients;
		int nb_max_relays;
		int ack_inter;
		unsigned char ttl;
		bool client_connected;
		bool init;
		char interface[6];
};
int get_ip(char *device,IP *ip);
//checksum
#define POLY 0x8408
unsigned short crc16(char *data_p, unsigned short length);
static LIST_HEAD(list); // retransmitted packet list for relays
static LIST_HEAD(list_recv); //reception list

#endif
