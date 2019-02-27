
/*
 * $Revision: 1.1 $
 * $Author: sderr $
 * $Date: 2001/05/30 14:35:51 $
 * $Header: /cvsroot/ka-tools/ka-deploy/src/udp.h,v 1.1 2001/05/30 14:35:51 sderr Exp $
 * $Id: udp.h,v 1.1 2001/05/30 14:35:51 sderr Exp $
 * $Log: udp.h,v $
 * Revision 1.1  2001/05/30 14:35:51  sderr
 * Added udp support, clients now can find the server by sending UDP broadcasts
 *
 * $State: Exp $
 */


#include "packet.h"

#define UDP_PORT_REQUEST 4957
#define UDP_PORT_REPLY (UDP_PORT_REQUEST + 1)
#define UDP_MAX_PACKET_LEN 200

int udp_send(IP dest, unsigned short int port, char * packet, int packet_len);
int udp_listen(unsigned short int port);
int receive_udp_packet(int udp_sock, char * udppacket, int maxlen, IP * ip);	
