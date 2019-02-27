


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
extern int pipe_write;
extern int ack_interval;

void mlp()
{
	start_external_command();
	RMTPClient *rmtpclient = new RMTPClient (&mcast);
	if (ack_interval) rmtpclient->setParam(SET_ACK_INTERVAL,ack_interval);
	rmtpclient->connect();
	rmtpclient->receive(pipe_write);
	while (rmtpclient->getStatus()!=RMTP_TRANSFER_SUCCESS) {
		if (rmtpclient->getStatus()==RMTP_TRANSFERING) {
			sleep(1);
		}
	}
	printf ("\nThe End\n");
	rmtpclient->disconnect();
	delete rmtpclient;
}

extern "C" {
void multicast_main_loop(){
	mlp();
}
}
