/*
 * $Revision: 1.3 $
 * $Author: mikmak $
 * $Date: 2001/07/01 15:27:18 $
 * $Header: /cvsroot/ka-tools/ka-deploy/rmtplib/RMTPserver.cc,v 1.3 2001/07/01 15:27:18 mikmak Exp $
 * $Id: RMTPserver.cc,v 1.3 2001/07/01 15:27:18 mikmak Exp $
 * $Log: RMTPserver.cc,v $
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

#include <iostream.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include "RMTPlib.h"
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <net/if.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>

RMTPServer::RMTPServer(IP *multicast)
{
	command_port=COMMAND_PORT;
	data_port=DATA_PORT;
	ttl=(u_char)TTL;
	mcast.s_addr=multicast->s_addr;
	timeout=TRANSMIT_TIMEOUT;
	nb_max_clients=MAX_CLIENTS_LEVEL;
	nb_max_relays=MAX_RELAYS_LEVEL;
	
	
	if (get_ip(INTERFACE,&my_ip) < 0 ) fprintf (stderr,"Error while trying to get my IP, if you use setInterface(), don't pay attention to this message\n");
	reset();
	if (initSockets(data_port,command_port,ttl) < 0) fprintf (stderr, "Error while initializing sockets\nIgnore this if you change parameters with setParam()\n");

	server_state=1; //server is up
	if ( pthread_create(&listener,NULL,(void *(*)(RMTPServer *))start_server, this) != 0) fprintf(stderr,"pthread_create\n");
}

RMTPServer::~RMTPServer()
{
	pthread_cancel(listener);
}

void
RMTPServer::reset()
{
	//reset everything but keeps clients connected
	int i;
#ifdef DEBUG 
	fprintf (stderr,"Resetting all values\n");
#endif
	seq_index=0;  
	last_seq_send=0;
	totaldata=0;
	nb_packs_retrans=0;
	totalretrans=0;
	syncing=false;
        memset (&window_send,0,sizeof(window_send));
	memset (&swindow_status,0,sizeof(swindow_status));
	memset (&ack_list, 0,RMTP_MAX_CLIENTS);
	memset (&list_sync, 0,RMTP_MAX_CLIENTS);
	for (i=0;i<nb_clients_total;i++) tree[i].receive_ready=false;
	server_status=RMTP_INIT;
	reading=false;
	sending=false;
	init=true;
	wait_discard=false;
	tree[0].id=1;
	tree[0].type=SERVER;
	tree[0].receive_ready=true;
	tree[0].ip_client.s_addr=my_ip.s_addr;
	tree[0].parent.s_addr=0;
	tree[0].level=0;
	wait_for_client=0;
	transfer_mode=0; // get back to default mode
}

int
RMTPServer::setParam(int param,int value)
{
	switch (param) {
		case SET_COMMAND_PORT:
			if (init) {
				command_port=(int)value;
#ifdef DEBUG
				fprintf (stderr,"Setting command port to %i\n",command_port);
#endif
				close(fd_data);
				close(fd_command);
				initSockets(data_port,command_port,ttl);
				return 0;
			} else return INIT_END;
			break;
		case SET_DATA_PORT:
			if (init) {
				data_port=(int)value;
#ifdef DEBUG
				fprintf (stderr,"Setting data port to %i\n",data_port);
#endif
				close(fd_data);
				close(fd_command);
				initSockets(data_port,command_port,ttl);
				return 0;
			break;
			} else return INIT_END;
		case SET_TIMEOUT:
			timeout=(int)value;
#ifdef DEBUG
			fprintf (stderr,"Setting timeout to %i\n",timeout);
#endif
			return 0;
			break;
		case SET_TTL:
			ttl=(u_char)value;
#ifdef DEBUG
			fprintf (stderr,"Setting TTL to %i\n",(int)ttl);
#endif
			if (setsockopt(fd_command,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl)) < 0) {
				fprintf(stderr,"setsockopt\n");
				 exit(1);
			}
                        if (setsockopt(fd_data,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl)) < 0) {
                                 fprintf(stderr,"setsockopt\n");
                                 exit(1);
                        }
			return 0;
			break;
		case SET_MAX_CLIENTS_LEVEL:
			if (!init) return INIT_END;
			if (client_connected) return CLIENT_CONNECTED;
			nb_max_clients=(int)value;
#ifdef DEBUG
			fprintf (stderr,"Setting max clients by level to %i\n",(int)nb_max_clients);
#endif
			return 0;
			break;		
		case SET_MAX_RELAYS_LEVEL:
			if (!init) return INIT_END;
			if (client_connected) return CLIENT_CONNECTED;
			nb_max_relays=(int)value;
#ifdef DEBUG
			fprintf (stderr,"Setting max relays by level to %i\n",(int)nb_max_relays);
#endif
			return 0;
			break;
		default :
			return -1;
	}
	return INVALID_PARAMETER;
}

int
RMTPServer::setInterface(char *iface)
{
	if (init) {
	        strncpy(interface,(char *)iface,6);
#ifdef DEBUG
        	fprintf (stderr,"Setting interface to %s\n",interface);
#endif
	        close (fd_command);
        	close (fd_data);
	        if (get_ip(interface,&my_ip) < 0) return -1;
        	if (initSockets(data_port,command_port,ttl) < 0) return -1;
	} else return INIT_END;
	return 0;
}

int
RMTPServer::RMTPsend(char *buf,int len, int clients=0)
{
	if (!init) return INIT_END;
	if (no_client) return NO_CLIENT;
	pthread_cancel(listener);
	reset();
	reading=true;
	sending=true;
	source_buf=buf;
	source_len=len;
	transfer_mode=1;
	wait_for_client=clients;
	initQueue();
	init=false;  // to block some setParam()
	gettimeofday(&start_time,0);
	
	start_server(this);
											
	// blocking mode now
	transfer_mode=0;
	//reset();
	server_status=RMTP_TRANSFER_SUCCESS;
	// normal mode
	if ( pthread_create(&listener,NULL,(void *(*)(RMTPServer *))start_server, this) != 0) fprintf(stderr,"pthread_create\n");
	return len;
}

int
RMTPServer::initSockets(int portdata,int portcommand,u_char ttl)
{
	struct ip_mreq mreq;
	struct sockaddr_in server,server_data;
	//int on=1;
	u_char loop=0;

	memset(&server,0,sizeof(server));
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	server.sin_port=htons(portcommand);
	memset(&server_data,0,sizeof(server_data));
	server_data.sin_family=AF_INET;
	server_data.sin_addr.s_addr=htonl(INADDR_ANY);
	server_data.sin_port=htons(portdata);
	if ((fd_command=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		fprintf(stderr,"socket_command\n");
		return -1;
	}
	if ((fd_data=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		fprintf(stderr,"socket_data\n");
		return -1;
	}
// 	if (setsockopt (fd_command, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0 ) {
//		fprintf(stderr,"setsockopt\n");
//		return -1;
//	}
//	if (setsockopt (fd_data, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0 ) {
//		fprintf(stderr,"setsockopt\n");
//		return -1;
//	}
	mreq.imr_multiaddr.s_addr=mcast.s_addr;
	mreq.imr_interface.s_addr=htonl(INADDR_ANY);
	if (setsockopt(fd_command,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
		fprintf(stderr,"setsockopt1\n");
		return -1;
	}
	if (setsockopt(fd_data,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
		fprintf(stderr,"setsockopt2\n");
		return -1;
	}
	if (setsockopt(fd_data, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
		fprintf(stderr,"setsockopt3\n"); 
		return -1;
	}
	if (setsockopt(fd_command, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) { 
		fprintf(stderr,"setsockopt4\n");
		return -1;
	}
        if (setsockopt(fd_data,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl)) < 0) {
                fprintf(stderr,"setsockopt5\n");
                return -1;
        }
        if (setsockopt(fd_command,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl)) < 0) {
                fprintf(stderr,"setsockopt6\n");
                return -1;
        }
	if (bind(fd_command,(struct sockaddr *) &server,sizeof(server)) < 0) {
		fprintf(stderr,"bind_command\n");
		return -1;
	}
	if (bind(fd_data,(struct sockaddr *) &server_data,sizeof(server_data)) < 0) {
		fprintf(stderr,"bind_data\n");
		return -1;
	}
	return 0;
}

void 
RMTPServer::sendPacketCommand(PacketCommand *pack)
{
	struct sockaddr_in server;

        memset(&server,0,sizeof(server));
        server.sin_family=AF_INET;
        server.sin_addr.s_addr=mcast.s_addr;
        server.sin_port=htons(command_port);

	if (sendto(fd_command,pack,sizeof(PacketCommand),0,(struct sockaddr *) &server,sizeof(server)) < 0) {
		fprintf(stderr,"sendto\n");
		exit(1);
	}

}

void 
RMTPServer::sendPacketData(PacketData *pack)
{
        struct sockaddr_in server;
	 
	memset(&server,0,sizeof(server));
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=mcast.s_addr;
	server.sin_port=htons(data_port);
					 
	last_send=time(NULL);
	
	totaldata+=pack->len;
#ifdef STATS
        if ((seq_index & 0xff) == 0) printStats();
#endif

	if (sendto(fd_data,pack,sizeof(PacketData),0,(struct sockaddr *) &server,sizeof(server)) < 0) {
		fprintf(stderr,"sendto\n");
		exit(1);
	}
}

void 
RMTPServer::processPacket(PacketCommand *pack) 
{
	switch (pack->type) {
		case GET_MASTER:
			newClient(pack);
			say(&pack->src,&my_ip,IAMMASTER);
			break;
		case RETRANSMIT:
			retransmit(pack);	
			break;
		case ACK:
			getAck(pack);
			break;
		case EOS:
			getSync(pack);
			break;
		case CLIENT_READY:
			clientReady(pack);
			break;
		case CLIENT_LEAVE:
			clientLeaving(pack);
			break;
		default:
			break;
	}
}

void
RMTPServer::statusWindow()
{
	int i,j;
	
	erasePacksInQueue();
	
	for (i=0;i<WINDOW_SIZE;i++) {
		fprintf (stderr,"Seq: %i\tSent: %i\t ACKs: ",(int)window_send[i].seq,swindow_status[i]);
		for (j=0;j<nb_clients;j++) fprintf (stderr,"%i ",ack_list[j]>=window_send[i].seq);
		fprintf (stderr,"\n");
	}	
	fprintf (stderr,"\n");
}

void
RMTPServer::clientLeaving(PacketCommand *pack)
{
	int i,j;
	//relays should not be able to leave if it still have clients
	// so when a relay disconnects , consider that all its clients disconnected before
	// a client disconnecting will send its request to its relay and not to the master
	// master will handle only its real clients

	//is it one of my client ?
	for (i=0;i<nb_clients;i++) {
		if (liste_client[i].ip_client.s_addr==pack->src.s_addr) { // yes :)
			for (j=i;j<nb_clients;j++) {
				if (liste_client[j+1].id) {
					memcpy (&liste_client[j],&liste_client[j+1],sizeof(Client));
					ack_list[j]=ack_list[j+1];
				} else memset (&liste_client[j],0,sizeof(Client));
			}
			memset (&liste_client[nb_clients],0,sizeof(Client));
			nb_clients--;
			if (nb_clients==0) {
				no_client=true;
				gettimeofday(&end_flow_time,0);
			}
			break;
		}
	}
	
	//delete it in the tree now
	deleteClientFromTree(&pack->src);
	fprintf (stderr,"Still %d clients connected\n",nb_clients_total);
}

void
RMTPServer::deleteClientFromTree(IP *ip)
{
	int i,j;
	bool is_relay=false,lvl=false;
	IP daddy;

	for (i=0;i<=nb_clients_total;i++) {
		if (tree[i].ip_client.s_addr==ip->s_addr) {
			if (tree[i].nb_child>0) is_relay=true;
			daddy.s_addr=tree[i].parent.s_addr;
			fprintf (stderr,"Removing client %i\n",tree[i].id);
			for (j=i;j<nb_clients_total;j++) memcpy (&tree[j],&tree[j+1],sizeof(Client));
			memset (&tree[nb_clients_total],0,sizeof (Client));
			nb_clients_total--;
			//look for its daddy to remove one child from list
			for (j=0;j<=nb_clients_total;j++) {
				if (tree[j].ip_client.s_addr==daddy.s_addr) {
					fprintf (stderr,"Dad found at %i with %i children\n",j,tree[j].nb_child);
					tree[j].nb_child--;
					if (tree[j].nb_child==0) {
						//check if the level still exists
						for (j=0;j<=nb_clients_total;j++) {
							if (tree[j].level==nb_level) {
								lvl=true;
								break;
							}
						}
						if (!lvl) nb_level--;
					}
				}
			}
#ifdef DEBUG
			fprintf (stderr, "Nb clients locaux : %i, total :%i Level :%i\n",nb_clients,nb_clients_total,nb_level);
#endif
			
			break;
		}
	}

	if (is_relay) {
		for (i=0;i<=nb_clients_total;i++) {
			if (tree[i].parent.s_addr==ip->s_addr) {
				deleteClientFromTree(&tree[i].ip_client);
			}
		}
	}
}

void
RMTPServer::printStats()
{
        int usec;
	float sec;
	float rate,userate;
			 
	printf("Data sent : %d Megs, Total data sent : %d Megs in %d packets and in %d retransmitted packets (%.3f%%)\n",
			(int) ((totaldata) >> 20),
			(int) ((totaldata+totalretrans) >> 20),
			(int) (seq_index + nb_packs_retrans),
			(int) nb_packs_retrans,
			(float)((float)(nb_packs_retrans * 100)/(float)(seq_index)) );
	if (end_flow_time.tv_sec==0) gettimeofday(&end_flow_time,0);
	usec = end_flow_time.tv_sec - start_time.tv_sec;
	usec = (usec * 1000000) + (end_flow_time.tv_usec - start_time.tv_usec);

	end_flow_time.tv_sec=0;
					 
	sec = ((float) usec) / 1000000.;
	rate =  ((float) (totaldata+totalretrans)) / sec;
	userate = ((float) (totaldata))/sec;
	printf("Transfer time = %.3f seconds, network throughput = %.3f Mbytes/second, data : %.3f Mbytes/sec\n", sec, rate/ (1024. * 1024.), userate/ (1024. * 1024.));
}


void 
RMTPServer::newClient(PacketCommand *pack)
{
	int i;
	if (tree[RMTP_MAX_CLIENTS-1].id) {
		fprintf (stderr, "Too many clients are trying to connect\n");
		return;
	}
	for (i=0;i<=nb_clients_total;i++) {
		if (tree[i].ip_client.s_addr==pack->src.s_addr) {
#ifdef DEBUG
			fprintf (stderr, "Client already registered this session\n");
#endif
			return;
		}
	}
	no_client=false;
	client_connected=true;
	buildTree(&pack->src);
}

void
RMTPServer::buildTree(IP *client)
{
	int i,current_level;
	int idx=-1;
	
	for (current_level=tree[0].level; current_level <= nb_level;current_level++) {
		for (i=current_level*(nb_max_clients+nb_max_relays); i<=nb_clients_total+1 && tree[i].id!=0;i++) {
			if ( (tree[i].type==SERVER || tree[i].type==CLIENT_RELAY) && tree[i].nb_child<(nb_max_clients+nb_max_relays)) {
				// so i found the highest - in terms of levels - relay which does not have been yet completed
				// so i'll full it :)
				idx=i;
				break;
			}
		}
		if (idx!=-1) break;
	}

	if (idx==-1) {
		// yup, i should not be there !
#ifdef DEBUG
		fprintf (stderr,"Something's wrong :/\n");
#endif
		exit(1);
	}

	//great, we are at the good level :)
	//now choose if we need a relay or a normal client
	if (tree[idx].nb_child<nb_max_clients) {
		//add a client
		nb_clients_total++;
		tree[nb_clients_total].id=nb_clients_total+1;
		tree[nb_clients_total].type=CLIENT_NORMAL;
		tree[nb_clients_total].parent.s_addr=tree[idx].ip_client.s_addr;
		tree[nb_clients_total].ip_client.s_addr=client->s_addr;
		tree[nb_clients_total].receive_ready=false;
		tree[nb_clients_total].nb_child=0;
		nb_level=MAX(tree[idx].level+1,nb_level);
		tree[nb_clients_total].level=nb_level;
		tree[idx].nb_child++;
		if (nb_level==1) {
			nb_clients++; // server's clients
			for (i=0;i<RMTP_MAX_CLIENTS;i++) {
				if (liste_client[i].id==0) {
					liste_client[i].id=i+1;
					liste_client[i].receive_ready=false;
					liste_client[i].ip_client.s_addr=client->s_addr;
#ifdef DEBUG
					fprintf (stderr,"Added local client %i \n",i+1);
					fprintf (stderr,"Nb clients local: %i, total : %i\n",nb_clients,nb_clients_total);
#endif
					break;
				}
			}
		}
#ifdef DEBUG
		fprintf (stderr,"Adding new client. My parent %i has %i children now. I am at level %i\n",tree[idx].id,tree[idx].nb_child,nb_level);
#endif
		if (tree[idx].nb_child==nb_max_clients+nb_max_relays) nb_level++;
	} else if (tree[idx].nb_child<(nb_max_clients+nb_max_relays)) {
		//add a relay
		nb_clients_total++;
		tree[nb_clients_total].id=nb_clients_total+1;
		tree[nb_clients_total].type=CLIENT_RELAY;
		tree[nb_clients_total].parent.s_addr=tree[idx].ip_client.s_addr;
		tree[nb_clients_total].ip_client.s_addr=client->s_addr;
		tree[nb_clients_total].receive_ready=false;
		tree[nb_clients_total].nb_child=0;
		nb_level=MAX(tree[idx].level+1,nb_level);
		tree[nb_clients_total].level=nb_level;
		tree[idx].nb_child++;
#ifdef DEBUG
		fprintf (stderr,"Adding new relay. My parent %i has %i children now.I am at level %i\n",tree[idx].id,tree[idx].nb_child,nb_level);
#endif
                if (nb_level==1) {
			nb_clients++; // server's clients
			for (i=0;i<RMTP_MAX_CLIENTS;i++) {
				if (liste_client[i].id==0) {
					liste_client[i].id=i+1;
					liste_client[i].receive_ready=false;
					liste_client[i].ip_client.s_addr=client->s_addr;
#ifdef DEBUG
					fprintf (stderr,"Added client %i \n",i+1);
					fprintf (stderr,"Nb clients : %i\n",nb_clients);
#endif
					break;
				}
			}
		}
		if (tree[idx].nb_child==nb_max_clients+nb_max_relays) nb_level++;
	} else {
#ifdef DEBUG
		fprintf (stderr,"PBL\n");
#endif
	}
	// the end :)
	// now say the client what it is and where it is in the tree
	say (client, &tree[idx].ip_client, tree[nb_clients_total].type);
#ifdef DEBUG
	fprintf (stderr,"My parent is %s\n",inet_ntoa(tree[idx].ip_client));
#endif
	// we should say the relay to connect the client now ...
	if (tree[idx].level!=0) {
	        PacketCommand p;
		struct sockaddr_in sin;
		p.type=NEW_CHILD;
		p.seq=0;
		p.src.s_addr=client->s_addr;
		sin.sin_port=htons(command_port);
		sin.sin_family=AF_INET;
		sin.sin_addr.s_addr=tree[idx].ip_client.s_addr;
		if (sendto(fd_command,&p,sizeof(PacketCommand),0,(struct sockaddr *) &sin,sizeof (struct sockaddr_in)) < 0) {
			fprintf(stderr,"error in sendto\n");
		}
	}
}

bool
RMTPServer::isrelay(IP *ip)
{
	int i;
	for (i=0;i<=nb_clients_total;i++) {
		if (tree[i].ip_client.s_addr==ip->s_addr && tree[i].type==CLIENT_RELAY) return true;
	}
	return false;
}

void 
RMTPServer::retransmit(PacketCommand *pack)
{
	int i;
	for (i=0;i<WINDOW_SIZE;i++) {
		if (window_send[i].seq==pack->seq) {
			sendRetrans(&window_send[i],&pack->src);
			return;
		}
	}
#ifdef DEBUG
	fprintf (stderr,"Packet is no longer in window (or not yet...), sorry...\n");
#endif
}

void 
RMTPServer::sendRetrans(PacketData *pack,IP *ip)
{
	struct sockaddr_in server;
	 
	memset(&server,0,sizeof(server));
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=ip->s_addr;
	server.sin_port=htons(command_port);

	totalretrans+=pack->len;
	nb_packs_retrans++;
	
	pack->type=RETRANSMITTED;
	if (sendto(fd_data,pack,sizeof(PacketData),0,(struct sockaddr *) &server,sizeof(server)) < 0) {
		fprintf(stderr,"sendto\n");
		exit(1);
	}
	pack->type=DATA_PACKET;
}

void
RMTPServer::say(IP *dest,IP *info,int mess)
{
        PacketCommand p;
        struct sockaddr_in sin;
		 
        p.type=mess;
        p.seq=0;
	p.src.s_addr=info->s_addr;
	 
        sin.sin_port=htons(command_port);
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=dest->s_addr;
 
	if (sendto(fd_command,&p,sizeof(PacketCommand),0,(struct sockaddr *) &sin,sizeof (struct sockaddr_in)) < 0) {
                fprintf(stderr,"error in sendto\n");
	}
}

void
RMTPServer::getSync(PacketCommand *pack)
{
	for (int i=0;i<nb_clients;i++) {
		if (pack->src.s_addr==liste_client[i].ip_client.s_addr) {
			list_sync[i] = pack->seq;
#ifdef DEBUG
			fprintf (stderr,"Client %i acknowledged end of sync\n",i);
#endif
		}
	}
	getAck(pack);
}

void 
RMTPServer::getAck(PacketCommand *pack)
{
	// in seq => a seq number indicating it successfully received all packets before this one
	int i;

	for (i=0;i<nb_clients;i++) {
		if (liste_client[i].ip_client.s_addr==pack->src.s_addr) {
			ack_list[i]=pack->seq;
			break;
		}
	}
	erasePacksInQueue();
}

void
RMTPServer::erasePacksInQueue()
{
	int j,i;
	bool acked;
	for (i=0;i<WINDOW_SIZE;i++) {
		acked=true;
		for (j=0;j<nb_clients;j++) {
			if (ack_list[j]<window_send[i].seq) {
				acked=false;
				break;
			}
		}
	
		if ((swindow_status[i]==true || last_seq_send >= window_send[i].seq) && acked) { 
			window_send[i].seq=0;
			swindow_status[i]=false;
		}
	}
}

void 
RMTPServer::clientReady(PacketCommand *pack)
{
	int i;

	for (i=0;i<nb_clients;i++) {
		if (liste_client[i].ip_client.s_addr==pack->src.s_addr) {
			liste_client[i].receive_ready=true;
			break;
		}
	}
	for (i=0;i<=nb_clients_total;i++) {
		if (tree[i].ip_client.s_addr==pack->src.s_addr) {
			tree[i].receive_ready=true;
#ifdef DEBUG
			fprintf (stderr,"Client %i is ready\n",i);
#endif
			break;
		}
	}
}

int 
RMTPServer::startTransfer(int source,int clients_connected=0)
{
	if (!init) return INIT_END;
	if (no_client && clients_connected==0) return NO_CLIENT;
	gettimeofday(&start_time,0);
	fd_source=source;
	initQueue();
	reading=true;
	sending=true;
	wait_for_client=clients_connected;
	transfer_mode=0;
	init=false;  // to block some setParam()
	fprintf (stderr,"Waiting for %d clients\n",clients_connected);
	return 1;
}


void 
start_server(RMTPServer *serv)
{
	fd_set rfds,wfds;
	struct timeval tv;
	time_t last_resent;

	int retval,i;
	bool end=true,readytogo=true,transfering=false;
	PacketCommand recvdCommand;
	
#ifdef DEBUG
	fprintf (stderr,"Size of PacketCommand : %i PacketData : %i\n",sizeof(PacketCommand),sizeof(PacketData));
#endif

	for(;;) {
		tv.tv_sec=0;
		tv.tv_usec=500000;
	
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(serv->fd_command, &rfds);
		//FD_SET(serv->fd_data, &wfds);

		readytogo=true;
		for (i=0;i<serv->nb_clients_total && !transfering;i++) {
			if (!serv->tree[i].receive_ready) {
				readytogo=false;
				break;	
			}
		}

		if (!transfering && readytogo && serv->nb_clients_total>=serv->wait_for_client && serv->sending) {
			transfering=true;
		}
		
		pthread_testcancel();

		if (serv->reading && serv->transfer_mode==0) {
			FD_SET(serv->fd_source, &rfds);
			retval = select (MAX(MAX(serv->fd_command,serv->fd_data),serv->fd_source)+1, &rfds, &wfds, NULL, &tv); 
		} else {
			retval = select (MAX(serv->fd_command,serv->fd_data)+1, &rfds, &wfds, NULL, &tv);
		}

		if (retval) {
			if (FD_ISSET(serv->fd_command, &rfds)) {
				read(serv->fd_command, &recvdCommand, sizeof(PacketCommand));	
				serv->processPacket(&recvdCommand);
				continue; 
			}
			if (serv->transfer_mode==1 || (serv->reading && serv->transfer_mode==0 && FD_ISSET(serv->fd_source, &rfds))) {
				serv->fillQueue();
			}
                        if (transfering && !serv->syncing && serv->sending /*&& FD_ISSET(serv->fd_data, &wfds)*/) {
				serv->server_status=RMTP_TRANSFERING;
				serv->transmit();
				time(&last_resent);	
			}
		}
		
		end=true;
		if (serv->nb_clients && !serv->sending && serv->seq_index) {
			for (int i=0;i < serv->nb_clients && !serv->sending && serv->seq_index;i++) {
				if (serv->ack_list[i]<serv->last_seq_send) {
					end=false;
					for (int j=0;j<WINDOW_SIZE;j++) {
						if ((serv->window_send[j]).seq==serv->last_seq_send && (time(NULL) - last_resent)>1) {
							serv->sendPacketData(&(serv->window_send[j]));
							time(&last_resent);
							break;
						}
					}
					break;
				}
			}
		}
		if (end && !serv->sending && serv->seq_index) {
			if (serv->transfer_mode==1) {
				serv->server_status=RMTP_TRANSFER_SUCCESS;	
				return;
			}
		}
#if 0
		if (!serv->sending && serv->seq_index && !serv->no_client) {
			//this means we're waiting for final ACKs
			for (int i=0;i<WINDOW_SIZE;i++) {
				//look for DATA_EOF
				if (serv->window_send[i].type==DATA_EOF) {
					//i resend it once every timeout until client asks for retransmits or disconnects !
					serv->sendPacketData(&(serv->window_send[i]));
				}
			}
		} else if (!serv->sending && serv->seq_index) {
			serv->reset();
		}
#endif
		if (serv->syncing) serv->checkSync();
	}
}

void
RMTPServer::shutdown()
{
	int i;
	for (i=0;i<=nb_clients_total;i++) {
		say(&tree[i].ip_client,&tree[i].ip_client,DIE);
	}
	close (fd_command);
	close (fd_source);
	close (fd_data);
	//clean tree list
	for (i=0;i<=nb_clients_total;i++) {
		memset(&tree[i],0,sizeof(Client));
	}
	//clean client list
	for (i=0;i<=nb_clients;i++) {
		memset(&liste_client[i],0,sizeof(Client));
	}
	reset();
	pthread_cancel(listener);
	server_state=0;//server is down
}

void
RMTPServer::restart()
{
	if (server_state!=0) {
		shutdown();
	}
	command_port=COMMAND_PORT;
	data_port=DATA_PORT;
	ttl=(u_char)TTL;
//	mcast.s_addr=multicast->s_addr;
	timeout=TRANSMIT_TIMEOUT;
	nb_max_clients=MAX_CLIENTS_LEVEL;
	nb_max_relays=MAX_RELAYS_LEVEL;

	if (get_ip(INTERFACE,&my_ip) < 0 ) fprintf (stderr,"Error while trying to get my IP, if you use setInterface(), don't pay attention to this message\n");
	if (initSockets(data_port,command_port,ttl) < 0) fprintf (stderr, "Error while initializing sockets\nIgnore this if you change parameters with setParam()");
	fprintf (stderr,"Restart\n");
	server_state=1; //server is up
	if ( pthread_create(&listener,NULL,(void *(*)(RMTPServer *))start_server, this) != 0) {
		fprintf(stderr,"pthread_create\n");
	}
}

void
RMTPServer::checkSync()
{
	bool temp=true;
	for (int i=0;i < nb_clients;i++) {
		if (list_sync[i] < syncseq) temp=false ; // my clients are not ready
	}
	syncing=temp;
	if (!syncing) {
#ifdef DEBUG
		fprintf (stderr,"EOS, sending....\n");
#endif
		last_send=time(NULL);
	}
								
}

void
RMTPServer::transmit()
{
	//send the last packet in queue available or nothing 
	int i,j;
	bool send=false;
	
	for (j=WINDOW_SIZE-1;j>=0;j--) {
		if (swindow_status[j]==false && window_send[j].seq!=0) {
			sendPacketData(&window_send[j]);
			last_seq_send++;
			send=true;
			if (window_send[j].type==DATA_EOF) {
				sending=false; // last packet sent, from now just wait ACKs and RETRANSMIT
			}
			swindow_status[j]=true; // ok, wait for ACK now...
			break;
		}
	}
	if (!send && (time(NULL)-last_send)>timeout) {
		//no send during the last second, guess we are waiting for ACKs which does not seem to come :/
		// i'll ask to the client its ACKs then wait, if no response then i'll ask a sync...
		//so, first i have to find the client which has pbls, look in list_ack
		last_send=time(NULL);
		if (!wait_discard) {
			fprintf (stderr, "Looking for laggy client !\n");
			for (i=0; i< nb_clients; i++) {
				//if (list_acks[WINDOW_SIZE-1][i]==0 ) {
				if (ack_list[i]<window_send[WINDOW_SIZE-1].seq && window_send[WINDOW_SIZE-1].seq!=0) {
					// this client has not send its ACKs, asking him to do so NOW
					say (&(liste_client[i].ip_client),&my_ip, ASK_ACK);
#ifdef DEBUG
					fprintf (stderr,"Client : %s is lagging\n",inet_ntoa(liste_client[i].ip_client));
#endif
					//resend last sent packet to make sure the client is aware of its status (ie being late)
					wait_discard=true;
				}
			}
			for (j=0;j<WINDOW_SIZE;j++) {
				if (window_send[j].seq==last_seq_send) {
					sendPacketData(&window_send[j]);
					break;
				}
			}
		} else {
			// no response from client, throw him away and go on !
			wait_discard=false;
			if (!syncing) {
				syncseq=last_seq_send;
				askSync(syncseq);
			}
		}
	}
}

void
RMTPServer::askSync(unsigned long seq)
{
	PacketCommand p;
	struct sockaddr_in sin;
#ifdef DEBUG
	fprintf (stderr,"Sending REQ_SYNC to %i\n",(int)seq);
#endif
	
	p.type=REQ_SYNC;
	p.seq=seq;
	p.src.s_addr=my_ip.s_addr;
	
	sin.sin_port=htons(command_port);
	sin.sin_family=AF_INET;
	syncing=true;
	for (int i=0;i<=nb_clients;i++) {
		sin.sin_addr.s_addr=liste_client[i].ip_client.s_addr;

		if (sendto(fd_command,&p,sizeof(PacketCommand),0,(struct sockaddr *) &sin,sizeof (struct sockaddr_in)) < 0) {
			fprintf(stderr,"error in sendto\n");
		}
	}
}

void
RMTPServer::initQueue()
{
	int i,n=0;
	for (i=WINDOW_SIZE-1;i>=0;i--) {
		//n=read(fd_source, &window_send[i].buff, PACKET_SIZE);
		n=readSource((char *)(&window_send[i].buff), PACKET_SIZE);
		window_send[i].seq=++seq_index;
		window_send[i].src.s_addr=my_ip.s_addr;
		window_send[i].len=n;
		window_send[i].crc=crc16(window_send[i].buff,n);
		if (n==0) {
			window_send[i].type=DATA_EOF;
			//fprintf (stderr,"Added DATA_EOF to queue seq %d\n",seq_index);
			break;
		}
		else window_send[i].type=DATA_PACKET;
	}
}

void
RMTPServer::fillGaps()
{
	for (int i=WINDOW_SIZE-1;i>=1;i--) {
		if (window_send[i].seq==0) {
			memcpy (&window_send[i], &window_send[i-1], sizeof(PacketData));
			window_send[i-1].seq=0;
			swindow_status[i]=swindow_status[i-1];
			swindow_status[i-1]=false;
		}
	}
}

int
RMTPServer::readSource(char *dest, int qty)
{ // we can add other sources if needed :)
	// typ_source : 0 for a fd and 1 for a buffer
	int n=-1;
	
	switch (transfer_mode) {
		case 0:
			n=read(fd_source, dest, qty);
			break;
		case 1:
			if (source_len>=qty) {
				memcpy(dest,source_buf,qty);
				source_buf+=qty; // move pointer
				source_len-=qty; // update len
				n=qty;
			} else {
				memcpy(dest,source_buf,source_len);
				source_buf+=source_len;
				n=source_len;
				source_len=0;
			}
			break;
		default:
			break;
	}
	return n;
}

void
RMTPServer::fillQueue()
{
	int i,n=0,idx=-1;

	erasePacksInQueue();
	fillGaps();

	for (i=WINDOW_SIZE-1;i>=0;i--) {
		if (window_send[i].seq==0) {
			idx=i;
			break;
		}
	}
	if (idx==-1) {
		return;
	}
	n=readSource((char *)&window_send[idx].buff, PACKET_SIZE);
	
	if (n==-1) { 
		fprintf (stderr, "Error while reading from fd_source\n"); 
	} else if (n==0) { 
		window_send[idx].type=DATA_EOF; 
		window_send[idx].seq=++seq_index; 
		window_send[idx].len=n;
		window_send[idx].crc=crc16(window_send[idx].buff,window_send[idx].len);
		window_send[idx].src.s_addr=my_ip.s_addr; 
		swindow_status[idx]=false;
		reading=false;
	} else {  
		window_send[idx].type=DATA_PACKET;
		window_send[idx].seq=++seq_index; 
		window_send[idx].len=n; 
		window_send[idx].crc=crc16(window_send[idx].buff,window_send[idx].len);
		window_send[idx].src.s_addr=my_ip.s_addr; 
		swindow_status[idx]=false;
	} 
}

void
RMTPServer::listQueue()
{
	for (int i=0; i<WINDOW_SIZE;i++) { 
		fprintf (stderr,"-->i: %i Seq : %i Status : %i \n", i, (int)window_send[i].seq,swindow_status[i]); 
	}
	
}

void
RMTPServer::listACKs()
{
	int i;
	for (i=0;i<nb_clients;i++) {
		fprintf (stderr,"Client %i :",i+1);
//		for (j=0;j<WINDOW_SIZE;j++) {
//			fprintf (stderr,"%i:%i ",(int)window_send[j].seq,list_acks[j][i]);
//		}
		fprintf (stderr,"\n");
	}
}

int
RMTPServer::getNbClients()
{
	return nb_clients_total;
}

int
RMTPServer::getStatus()
{
	return server_status;
}
