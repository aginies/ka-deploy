
/*
 * $Revision: 1.2 $
 * $Author: mikmak $
 * $Date: 2001/07/01 15:27:18 $
 * $Header: /cvsroot/ka-tools/ka-deploy/src/udp.c,v 1.2 2001/07/01 15:27:18 mikmak Exp $
 * $Id: udp.c,v 1.2 2001/07/01 15:27:18 mikmak Exp $
 * $Log: udp.c,v $
 * Revision 1.2  2001/07/01 15:27:18  mikmak
 * multicast is now integrated and working. Many updates to rmtplib and optimization for Ka.Still much to do ...
 *
 * Revision 1.1  2001/05/30 14:35:51  sderr
 * Added udp support, clients now can find the server by sending UDP broadcasts
 *
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>




#include "udp.h"

int udp_send(IP dest, unsigned short int port, char * packet, int packet_len)
{
	int sockfd;
	struct sockaddr_in their_addr;
	int on = 1;
	int numbytes;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		return -1;
	}

	
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));

	their_addr.sin_family = AF_INET;         /* host byte order */
	their_addr.sin_port = htons(port);     /* short, network byte order */
	their_addr.sin_addr = dest;
	bzero(&(their_addr.sin_zero), 8);        /* zero the rest of the struct */
	
	
	if ((numbytes=sendto(sockfd, packet, packet_len,0,(struct sockaddr *)&their_addr,
                	 sizeof(struct sockaddr))) == -1) {
		perror("sendto");
		return -1;
	}
	
	close(sockfd);
	
	return numbytes;
}

		
int udp_listen(unsigned short int port)
{
	int sockfd;
	struct sockaddr_in my_addr;    /* my address information */
    
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		return -1;
	}

	my_addr.sin_family = AF_INET;         /* host byte order */
	my_addr.sin_port = htons(port);     /* short, network byte order */
	my_addr.sin_addr.s_addr = INADDR_ANY; /* automatically fill with my IP */
	bzero(&(my_addr.sin_zero), 8);        /* zero the rest of the struct */

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		return -1;
	}

	return sockfd;
}
	
	
int receive_udp_packet(int udp_sock, char * udppacket, int maxlen, IP * ip)	
{
	struct sockaddr_in their_addr; /* connector's address information */
	socklen_t addr_len;
	int numbytes;
	
	addr_len = sizeof(struct sockaddr);

	numbytes =  recvfrom(udp_sock ,udppacket, maxlen,0,(struct sockaddr *)&their_addr,
                		   &addr_len);
				   
	*ip = their_addr.sin_addr;
	
	return numbytes;
}
