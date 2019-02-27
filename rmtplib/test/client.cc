/*
 * $Revision: 1.1 $
 * $Author: mikmak $
 * $Date: 2001/06/29 12:04:58 $
 * $Header: /cvsroot/ka-tools/ka-deploy/rmtplib/test/client.cc,v 1.1 2001/06/29 12:04:58 mikmak Exp $
 * $Id: client.cc,v 1.1 2001/06/29 12:04:58 mikmak Exp $
 * $Log: client.cc,v $
 * Revision 1.1  2001/06/29 12:04:58  mikmak
 * added
 *
 * $State: Exp $
 */
	
#include "../RMTPlib.h"
#include <iostream.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char ** argv) 
{
	IP me;
	int fd_out;
	char *buff=(char *)malloc(400);
	
	me.s_addr=inet_addr("226.6.6.66");
	unlink("/tmp/K");
	fd_out=open("/tmp/K",O_CREAT | O_RDWR,S_IRWXU);
	RMTPClient *client=new RMTPClient(&me);
	client->connect();
//	client->receive(fd_out);
//	sleep(10);
//	client->RMTPrecv(buff,40);
//	printf ("%s\n",buff);
	client->receive(fd_out);
	sleep(600);
	client->disconnect();
	return 0;
}

