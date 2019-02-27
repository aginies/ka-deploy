


#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>

#include "packet.h"
#include "tree.h"

#include "../rmtplib/RMTPlib.h"



/* from server.c : */
extern "C" { void start_external_command(); }
extern IP mcast;
extern int pipe_read;
extern int wanted_clients;
extern int win_size;

void mlp()
{
	start_external_command();
	RMTPServer *master = new RMTPServer (&mcast); // waiting for clients to connect here;
	if (win_size) master->setParam (SET_WINSIZE,win_size);
	master->startTransfer(pipe_read,wanted_clients);
	while (master->getStatus()!=RMTP_TRANSFER_SUCCESS) {
		if (master->getStatus()==RMTP_TRANSFERING) {
			sleep(1);
		}
	}
	printf ("\nThe End\n");
	delete master;	
}

extern "C" {
void multicast_main_loop()
{
	mlp();
}
}
