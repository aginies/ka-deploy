/*
 * $Revision: 1.1 $
 * $Author: mikmak $
 * $Date: 2001/06/29 12:04:58 $
 * $Header: /cvsroot/ka-tools/ka-deploy/rmtplib/test/server.cc,v 1.1 2001/06/29 12:04:58 mikmak Exp $
 * $Id: server.cc,v 1.1 2001/06/29 12:04:58 mikmak Exp $
 * $Log: server.cc,v $
 * Revision 1.1  2001/06/29 12:04:58  mikmak
 * added
 *
 * $State: Exp $
 */

#include "../RMTPlib.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <iostream.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	IP me;
	int ft;
	int ttl=64;
	int timeout=1;
	int relays=10;
	int clients=10;
	char buff[]="test test";

	me.s_addr=inet_addr("226.6.6.66");
	RMTPServer *myserv = new RMTPServer(&me);
	ft=open("/tmp/test-multi",0);
//	myserv->RMTPsend(buff,sizeof(buff),atoi(argv[1]));
	//myserv->RMTPsend(buff,sizeof(buff),atoi(argv[1]));
	//sleep(1);
	myserv->startTransfer(ft,atoi(argv[1]));
	sleep(600);
	return 0;
}
