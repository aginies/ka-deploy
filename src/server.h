
#ifndef SERVER_H_INC
#define SERVER_H_INC

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include "packet.h"
#include "tree.h"
#include "buffer.h"


#define PRINT_INFO
#define PRINT_WARN
#define PRINT_DEBUG

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

struct connection;

struct client {
	struct connection * conn;
	IP ip;
	struct client * children[ARITE_MAX];
	struct client * daddy;
	int nb_children;
	/* number of nodes levels under this node in the tree, for tree balance */
	int levels_under;
	int busy;
	int data_connected; // has a data connection to his daddy ?
	int auth_done;
	int answered_ping;
	int dropped;
	unsigned char will_wait_for_others; //bool
	offset_t data_offset;
	int dad_wants_position;
	int is_waiting_release;
	struct client * next_waiting_client;
	struct timeval release_date;
};





#endif /* SERVER_H_INC */
