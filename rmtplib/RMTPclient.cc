/*
 * $Revision: 1.3 $
 * $Author: mikmak $
 * $Date: 2001/07/01 15:27:18 $
 * $Header: /cvsroot/ka-tools/ka-deploy/rmtplib/RMTPclient.cc,v 1.3 2001/07/01 15:27:18 mikmak Exp $
 * $Id: RMTPclient.cc,v 1.3 2001/07/01 15:27:18 mikmak Exp $
 * $Log: RMTPclient.cc,v $
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
#include <string.h>


RMTPClient::RMTPClient(IP *multicast)
{
	strncpy(interface,INTERFACE,6);
	fprintf (stderr,"Interface : %s\n",interface);
	if (get_ip(interface,&my_ip) <0) fprintf (stderr,"Error while trying to get my IP, if you use setInterface(), don't pay attention to this message\n");
	reset();
	fd_dest=-1;
	mcast.s_addr=multicast->s_addr;
	command_port=COMMAND_PORT;
	data_port=DATA_PORT;
	ttl=(u_char)TTL;
	ack_inter=ACK_INTERVAL;
	if (initSockets(data_port,command_port,ttl) < 0) fprintf (stderr, "Error while initializing sockets\nIgnore this if you change parameters with setParam()");
	initQueue();
	
        if ( pthread_create(&listener,NULL,(void *(*)(RMTPClient *))start_client, this) != 0) fprintf(stderr,"pthread_create\n");
	
}

RMTPClient::~RMTPClient()
{
	pthread_cancel(listener);	
}

void
RMTPClient::receive(int fd)
{
	fd_dest=fd;
	transfer_mode=0;
	receiving=true;
	sayReady();
}

void
RMTPClient::reset()
{
	struct list_head *scan;
	nb_packet_received=0;
	last_seq_completed=0;
	receiving=true;
	syncing=false;
	syncseq=0;
	init=true;
	client_status=RMTP_INIT;
	wait=-1;	
	
        list_for_each(scan, &list_recv) {
                PacketList *p=list_entry(scan,PacketList,l);
	        list_del(&p->l);
		delete p;
        }
        list_for_each(scan, &list) {
                PacketList *p=list_entry(scan,PacketList,l);
	        list_del(&p->l);
		delete p;
        }
	memset (&ack_list,0,RMTP_MAX_CLIENTS);
	memset (&list_sync,0,RMTP_MAX_CLIENTS);
	transfer_mode=0;
	fd_dest=-1;
}

int
RMTPClient::initSockets(int dataport,int commandport, u_char ttl)
{
        struct sockaddr_in client,client_data;
        struct ip_mreq mreq;
	u_char loop=0;
	//int on=1;
	
	memset(&client,0,sizeof(client));
	client.sin_family=AF_INET;
	client.sin_addr.s_addr=htonl(INADDR_ANY);
	client.sin_port=htons(COMMAND_PORT);
	
	memset(&client_data,0,sizeof(client_data));
	client_data.sin_family=AF_INET;
	client_data.sin_addr.s_addr=htonl(INADDR_ANY);
	client_data.sin_port=htons(DATA_PORT);

	if ((fd_command=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		fprintf(stderr,"socket_command\n");
		return -1;
	}

	if ((fd_data=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		fprintf(stderr,"socket_data\n");
		return -1;
	}
#if 0
	if (setsockopt (fd_command, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0 ) {
		fprintf(stderr,"setsockopt\n");
		exit(1);
	}
	if (setsockopt (fd_data, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0 ) { 
		 fprintf(stderr,"setsockopt\n");
		 exit(1);
	}
#endif
	mreq.imr_multiaddr.s_addr=mcast.s_addr;
	mreq.imr_interface.s_addr=htonl(INADDR_ANY);

	if (setsockopt(fd_command,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
		fprintf(stderr,"setsockopt\n");
		return -1;
	}
        if (setsockopt(fd_data,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
		fprintf(stderr,"setsockopt\n");
		return -1;
	}
	if (setsockopt(fd_command, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
		fprintf (stderr, "setsockopt\n");
		return -1;
	}
	if (setsockopt(fd_command, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
		fprintf (stderr, "setsockopt\n");
		return -1;
	}
        if (setsockopt(fd_data,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl)) < 0) {
		fprintf(stderr,"setsockopt\n");
		return -1;
	}
	if (setsockopt(fd_command,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl)) < 0) {
		fprintf(stderr,"setsockopt\n");
		return -1;
	}
        if (bind(fd_command,(struct sockaddr *) &client,sizeof(client)) < 0) {
		fprintf(stderr,"bind_command\n");
		return -1;
	}
	if (bind(fd_data,(struct sockaddr *) &client_data,sizeof(client_data)) < 0) {
		fprintf(stderr,"bind_data\n");
                return -1;
	}
	return 0;
}

int
RMTPClient::setParam(int param,int value)
{
	switch (param) {
		case SET_ACK_INTERVAL:
		ack_inter=(int)value;
#ifdef DEBUG
		fprintf (stderr,"Setting ACK to %i\n",ack_inter);
#endif
		break;
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
		case SET_TTL:
			ttl=(unsigned char)value;
#ifdef DEBUG
			fprintf (stderr,"Setting TTL to %i\n",(int)ttl);
#endif
			if (setsockopt(fd_command,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl)) < 0) {
				fprintf(stderr,"setsockopt\n");
				return WRONG_TTL;
			}	
			if (setsockopt(fd_data,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl)) < 0) {
				fprintf(stderr,"setsockopt\n");
				return WRONG_TTL;
			}
			break;
		default :
			return -1;
	}
	return INVALID_PARAMETER;
}

int
RMTPClient::setInterface(char *iface)
{
	if (init) {
		strncpy(interface,(char *)iface,6);
#ifdef DEBUG
		fprintf (stderr,"Setting interface to %s\n",interface);
#endif
		close (fd_command);
		close (fd_data);
		if (get_ip(interface,&my_ip) < 0) {
			fprintf (stderr,"Wrong interface\n");	
			return WRONG_INTERFACE;
		}
		initSockets(data_port,command_port,ttl);
	} else return INIT_END;
	return 0;
}

void
RMTPClient::statusWindow()
{
	int j;
	struct list_head *scan;
	
	erasePacksInQueue();

	list_for_each(scan, &list_recv) {
		PacketList *p=list_entry(scan,PacketList,l);
		fprintf (stderr,"Seq: %i\t Sent: %i\t ACKs: ",(int)(p->pkt).seq,p->status); 
		for (j=0;relay && j<nb_clients;j++) fprintf (stderr,"%i ",ack_list[j]>=(p->pkt).seq);
		fprintf(stderr,"\n");
	}
}

void
RMTPClient::newClient(PacketCommand *pack)
{
	int i;

	if (!relay) {
		fprintf (stderr,"Sommething is wrong, i should not receive new client !\n");
		exit(1);
	}
	for (i=0;i<RMTP_MAX_CLIENTS;i++) {
		if (liste_client[i].id==0) {
			no_client=false;
			liste_client[i].id=i+1;
			liste_client[i].receive_ready=false;
			liste_client[i].ip_client.s_addr=pack->src.s_addr;
			nb_clients++;
#ifdef DEBUG
			fprintf (stderr,"Added client %i \n",i+1);
			fprintf (stderr,"Nb clients : %i\n",nb_clients);
#endif
			break;
		}
	}
}

void
RMTPClient::clientLeaving(PacketCommand *pack)
{
	int i,j;
	for (i=0;i<=nb_clients;i++) {
		if (liste_client[i].ip_client.s_addr==pack->src.s_addr) { 
			for (j=i;j<nb_clients;j++) {
				if (liste_client[j+1].id) {
					memcpy (&liste_client[j],&liste_client[j+1],sizeof(Client));
					ack_list[j]=ack_list[j+1];
				} else memset (&liste_client[j],0,sizeof(Client)); 
			}
			memset (&liste_client[nb_clients],0,sizeof(Client));
			nb_clients--;
#ifdef DEBUG
			fprintf (stderr, "Nb clients locaux : %i\n",nb_clients);
#endif
			if (nb_clients==0) no_client=true;
			break;
		}
	}
}

void
RMTPClient::say(int what,IP *to)
{
        PacketCommand p; 
	struct sockaddr_in sin;
	 
	p.type=what;
	p.seq=0;
	p.src.s_addr=my_ip.s_addr;

	sin.sin_port=htons(COMMAND_PORT);
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=to->s_addr;

	if (sendto(fd_command,&p,sizeof(PacketCommand),0,(struct sockaddr *) &sin,sizeof (struct sockaddr_in)) < 0) fprintf(stderr,"error in sendto\n");
}

void
RMTPClient::sayReady()
{//second
	say(CLIENT_READY,&parent);
	say(CLIENT_READY,&master);	
}

void
RMTPClient::connect()
{//first
	say(GET_MASTER,&mcast);
}

void
RMTPClient::disconnect()
{//last
	say(CLIENT_LEAVE,&parent);
	say(CLIENT_LEAVE,&master);
}

void
RMTPClient::retransmit(long seq)
{
	PacketCommand p;
        struct sockaddr_in sin; 
	p.type=RETRANSMIT;
	p.seq=seq;
	p.src.s_addr=my_ip.s_addr; 
        sin.sin_port=htons(COMMAND_PORT);
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=parent.s_addr;
        if (sendto(fd_command,&p,sizeof(PacketCommand),0,(struct sockaddr *) &sin,sizeof (struct sockaddr_in)) < 0) fprintf(stderr,"error in sendto\n");
#ifdef DEBUG	
	fprintf (stderr,"Asking retransmission of packet seq %i\n",(int)seq);
#endif
}

int
RMTPClient::RMTPrecv(char *buf,int len)
{
	if (!init) return INIT_END;
	//check server ?
	transfer_mode=1;
	dest_buff=buf;
	dest_len=len;
	init=false;	

	pthread_cancel(listener);
	sayReady();
	start_client(this);
	transfer_mode=0;
	reset();
	if (pthread_create(&listener,NULL,(void *(*)(RMTPClient *))start_client, this) != 0) fprintf(stderr,"pthread_create\n");
	return len;
}

void
start_client(RMTPClient *client)
{
	fd_set rfds, wfds;
	struct timeval tv;
	PacketData recvdData;
	int retval;
	time_t last_ready;
	time(&last_ready);
	int nb_ready=0;

	fprintf (stderr,"Listening ...\n");
	//client->receiving=true;
	while (1){
		tv.tv_sec=0;
		tv.tv_usec=500000;
		pthread_testcancel();

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(client->fd_command, &rfds);
		FD_SET(client->fd_data, &rfds);
		//if (client->fd_dest!=-1) {
		//	FD_SET(client->fd_dest, &wfds);
		//	retval = select (MAX(MAX(client->fd_command,client->fd_data),client->fd_dest)+1, &rfds, &wfds, NULL, &tv);
		/*} else*/ retval = select (MAX(client->fd_command,client->fd_data)+1, &rfds, NULL, NULL, &tv);
		
		if (retval) {
			if (FD_ISSET(client->fd_command, &rfds)) {
				read(client->fd_command,&recvdData,sizeof(PacketData));
				client->processPacket(&recvdData);
			}
			if (FD_ISSET(client->fd_data, &rfds)) {
				client->init=false;
				client->client_status=RMTP_TRANSFERING;
				client->fillQueue();
			}
			if ((client->fd_dest!=-1 /*&& FD_ISSET(client->fd_dest, &wfds)*/ && client->receiving && client->transfer_mode==0) || client->transfer_mode==1) {
				client->transmitToDest();
			}
		}
		if ( ( client->transfer_mode==1 || ( client->transfer_mode==0 && client->fd_dest!=-1 ) && client->last_seq_completed==0 ) && ( time ( NULL ) - last_ready ) > 1 && nb_ready<=0 ) {
			time(&last_ready);
			nb_ready++;
			client->sayReady();
		}
		if (!client->receiving && client->transfer_mode==1) {
			client->client_status=RMTP_TRANSFER_SUCCESS;
			return;
		}
		if (client->syncing) client->checkSync();
	}
}
#if 0
void
start_relay(RMTPClient *client)
{
        fd_set rfds,wfds;
        struct timeval tv;
        PacketData recvdData;

        int retval;
        fprintf (stderr,"Listening....\n");
        //client->receiving=true;
        while (1/*client->nb_clients*/) {
		tv.tv_sec=0;
		tv.tv_usec=200;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(client->fd_command, &rfds);
		//FD_SET(client->fd_command, &wfds);
		FD_SET(client->fd_dest, &wfds);
		FD_SET(client->fd_data, &rfds);
		pthread_testcancel();
		retval = select (MAX(MAX(client->fd_command,client->fd_data),client->fd_dest)+1, &rfds, &wfds, NULL, &tv);
		if (retval) {
			if (FD_ISSET(client->fd_command, &rfds)) {
				read(client->fd_command,&recvdData,sizeof(PacketData));
				client->processPacket(&recvdData);
			}
			if (FD_ISSET(client->fd_data, &rfds)) {
				client->init=false;
				client->fillQueue();
			}
			if ( FD_ISSET(client->fd_dest, &wfds) ) {
				client->transmitToDest();
			}
		}
		if (client->syncing) client->checkSync();
	}
}

void
start_client(RMTPClient *client)
{
	fd_set rfds,wfds;
	struct timeval tv;
	PacketData recvdData;
	
        int retval;
        fprintf (stderr,"Listening....\n");
	client->receiving=true;
        while (/*client->receiving*/1) {
                tv.tv_sec=0;
                tv.tv_usec=200;
					 
                FD_ZERO(&rfds);
		FD_ZERO(&wfds);
	        FD_SET(client->fd_command, &rfds);
		//FD_SET(client->fd_command, &wfds);
		FD_SET(client->fd_dest, &wfds);
	        FD_SET(client->fd_data, &rfds);
		pthread_testcancel();

		retval = select (MAX(MAX(client->fd_command,client->fd_data),client->fd_dest)+1, &rfds, &wfds, NULL, &tv);
		if (retval) {
		        if (FD_ISSET(client->fd_data, &rfds)/* && !client->isfull()*/) {
				client->init=false;
				client->fillQueue();
		        }
			if (FD_ISSET(client->fd_command, &rfds)) {
				read(client->fd_command,&recvdData,sizeof(PacketData));
				client->processPacket(&recvdData);
			}
			if ( FD_ISSET(client->fd_dest, &wfds) && client->receiving) {
				client->transmitToDest();
			}
		}
		if (client->syncing) client->checkSync();
	}
}
#endif

void
RMTPClient::checkSync()
{
	if (relay) {
		for (int i=0;i < nb_clients;i++) {
			if (list_sync[i] < syncseq) return ; // my clients are not ready
		}
	}
	if (last_seq_completed>=syncseq) {
		sendSync();
		syncing=false;
	}
}

int
RMTPClient::writeDest(char *src, int qty)
{
	int n=-1;

	switch (transfer_mode) {
		case 0:
			n=write(fd_dest,src,qty);
			break;
		case 1:
			if (qty<=dest_len) {
				memcpy(dest_buff,src,qty);
				dest_len-=qty;
				dest_buff+=qty;
				n=qty;
			} else {
				memcpy(dest_buff,src,dest_len);
				dest_buff+=dest_len;
				n=dest_len;
				dest_len=0;
			}
			break;
		default:
			break;
	}
	return n;
}

void
RMTPClient::sendSync()
{
	PacketCommand p;
	struct sockaddr_in sin;
	
#ifdef DEBUG
	fprintf (stderr,"Sending EOS\n");
#endif
	
	p.type=EOS;
	p.seq=syncseq;
	p.src.s_addr=my_ip.s_addr;
	
	sin.sin_port=htons(COMMAND_PORT);
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=parent.s_addr;
	
	if (sendto(fd_command,&p,sizeof(PacketCommand),0,(struct sockaddr *) &sin,sizeof (struct sockaddr_in)) < 0) {
		fprintf(stderr,"error in sendto\n");
	}
}

bool
RMTPClient::isfull()
{
	return false; //the list cannot be full hopefully, needs some testing to confirm ;)
#if 0
	int i;
	for (i=0;i<RCV_WINDOW;i++) {
		if (window_recv[i].seq==0) return false;
	}
	return true;
#endif
}

void
RMTPClient::clientReady(PacketCommand *pack)
{
	int i;
	for (i=0;i<nb_clients;i++) {
		if (liste_client[i].ip_client.s_addr==pack->src.s_addr) {
			liste_client[i].receive_ready=true;
#ifdef DEBUG
			fprintf (stderr,"Client %i is ready\n",i+1);
#endif
		}
	}
}

void
RMTPClient::processPacket(PacketData *pack)
{
	int n,i;
	unsigned long min,max,k;
	PacketList *packet;
	struct list_head *scan;

	switch (((PacketCommand*)pack)->type) {
		case IAMMASTER:
			master.s_addr=((PacketCommand*)pack)->src.s_addr;
#ifdef DEBUG
			fprintf (stderr,"My master is %s\n",inet_ntoa(master));
#endif
			break;
		case REQ_SYNC:
			syncing=true;
			syncseq=((PacketCommand*)pack)->seq;
#ifdef DEBUG
			fprintf(stderr,"Master asked a SYNC to %i\n",(int)syncseq);
#endif
			if (relay) askSync(syncseq);
			break;
		case NEW_CHILD:
			newClient((PacketCommand*)pack);
			break;
		case CLIENT_LEAVE:
			clientLeaving((PacketCommand*)pack);
			break;
		case CLIENT_READY:
			clientReady((PacketCommand*)pack);
			break;
		case DATA_EOF:
#ifdef DEBUG
			fprintf (stderr, "The end\n");
#endif
			last_seq_completed++;
			sendAck();
			break;
		case ACK:
			getAck((PacketCommand*)pack);
			break;
		case EOS:
			getSync((PacketCommand*)pack);
			break;
		case RETRANSMIT:
			resend((PacketCommand*)pack);
			break;
		case RETRANSMITTED:
			//i try to add it in the queue to be able to resend it to my clients if necessary
			// check its crc first
			if (pack->crc!=crc16(pack->buff,pack->len)) return;
			bool stop;
			if (relay) {
				stop=false;
			
				list_for_each(scan, &list) {
					PacketList *p=list_entry(scan,PacketList,l);
					if ((p->pkt).seq==pack->seq) stop=true;
				}
				if (!stop) {
					packet=new PacketList();
					memcpy(&(packet->pkt), pack,sizeof(PacketData));
					(packet->pkt).type=DATA_PACKET;
					list_add(&packet->l,&list);
#ifdef DEBUG
					fprintf (stderr,"Added packet %d , %d retr to queue\n",(int)(packet->pkt).seq,(int)pack->seq);
#endif
				}
			}
			if (pack->seq==last_seq_completed+1) {
				last_seq_completed++;
				if (last_seq_completed % ack_inter == 0) sendAck();
				wait=-1;
        	                //n=write(fd_dest,(pack)->buff,(pack)->len);
				n=writeDest((pack)->buff,(pack)->len);
				if (n==-1) fprintf (stderr,"Write error to fd_dest\n");
#ifdef DEBUG
				else fprintf (stderr,"Wrote retransmitted pack\n");
#endif
			}
			
			break;
		case ASK_ACK:
#ifdef DEBUG
			fprintf (stderr,"Server asks my ACKs :).I am at %d\n",(int)last_seq_completed);
#endif
			sendAck();
			min=0;
			max=0;
			list_for_each (scan, &list_recv) {
				PacketList *p=list_entry(scan,PacketList,l);
				if ((p->pkt).seq<min || min==0) min=(p->pkt).seq;
				if ((p->pkt).seq>max || max==0) max=(p->pkt).seq;
			}
			if (min>last_seq_completed+1) {
				for (k=last_seq_completed+2;k<=min;k++) retransmit(k);
			}
			if (relay) {
				for (i=0;i<nb_clients;i++) {
					say(ASK_ACK,&(liste_client[i].ip_client));
					list_for_each (scan, &list_recv) {
						PacketList *p=list_entry(scan,PacketList,l);
						if ((p->pkt).seq==max) {
							sendRetrans(&p->pkt,&(liste_client[i].ip_client));
							break;
						}
					}
				}
			}
#if 0
			mn=findMin();
			max=findMax();
			if (window_recv[mn].seq>last_seq_completed+1) {
				for (k=last_seq_completed+2;k<=window_recv[mn].seq;k++) {
					 retransmit (k);
				}
			}
			if (relay) for (i=0;i<nb_clients;i++) {
				say(ASK_ACK,&(liste_client[i].ip_client));
				sendRetrans(&window_recv[max],&(liste_client[i].ip_client));
			}
#endif
			break;
		case CLIENT_NORMAL:
			parent.s_addr=((PacketCommand *)pack)->src.s_addr;			
			relay=false;
#ifdef DEBUG
			fprintf (stderr,"OK, i am a normal client, my parent is %s\n",inet_ntoa(parent));
#endif
			//sayReady();
			break;
		case CLIENT_RELAY:
			parent.s_addr=((PacketCommand *)pack)->src.s_addr;
			relay=true;
#ifdef DEBUG
			fprintf (stderr,"OK, i am a relay, my parent is %s\n",inet_ntoa(parent));
#endif
			//sayReady();
			break;
		case DIE:
#ifdef DEBUG
			fprintf (stderr,"Server asked me to leave, killing myself...\n");
#endif
			disconnect();
			close (fd_dest);
			close (fd_command);
			close (fd_data);
			exit(1);
	}
}

void
RMTPClient::askSync(unsigned long seq)
{
	PacketCommand p;
	struct sockaddr_in sin;
	if (seq==0) return;
#ifdef DEBUG
	fprintf (stderr,"Sending REQ_SYNC to %i\n",(int)seq);
#endif
			        
	p.type=REQ_SYNC;
	p.seq=seq;
	p.src.s_addr=my_ip.s_addr;
	syncing=true;
	for (int i=0;i<nb_clients;i++) {
		sin.sin_port=htons(COMMAND_PORT);
		sin.sin_family=AF_INET;
		sin.sin_addr.s_addr=liste_client[i].ip_client.s_addr;

		if (sendto(fd_command,&p,sizeof(PacketCommand),0,(struct sockaddr *) &sin,sizeof (struct sockaddr_in)) < 0) {
			fprintf(stderr,"error in sendto\n");
		}	
	}
}

void
RMTPClient::getSync(PacketCommand *pack)
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

int
RMTPClient::findMax()
{
#if 0
	int i,max=0;
	for (i=0;i<RCV_WINDOW;i++) {
		if (window_recv[i].seq>window_recv[max].seq && window_recv[i].seq!=0) max=i;
		if (window_recv[max].seq==0 && window_recv[i].seq!=0) max=i;
	}
	return max;
#endif
return 0;
}
int 
RMTPClient::findMin()
{
#if 0
	int i,min=0;
	for (i=0;i<RCV_WINDOW;i++) {
		if (window_recv[i].seq<window_recv[min].seq && window_recv[i].seq!=0) min=i;
		if (window_recv[min].seq==0 && window_recv[i].seq!=0) min=i;
	}
	return min;
#endif
return 0;
}

void
RMTPClient::getAck(PacketCommand *pack)
{
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
RMTPClient::resend (PacketCommand *pack)
{
	struct list_head *scan;

	list_for_each(scan, &list) {
		PacketList *p = list_entry(scan,PacketList,l);
		if ((p->pkt).seq==pack->seq) {
			sendRetrans(&p->pkt,&pack->src);
#ifdef DEBUG
			fprintf (stderr,"Resending packet from retr queue seq %i\n",(int)pack->seq);
#endif
			return;
		}
	}

	list_for_each (scan, &list_recv) {
		PacketList *p= list_entry(scan,PacketList,l);
		if ((p->pkt).seq==pack->seq) {
			sendRetrans(&p->pkt,&pack->src);
#ifdef DEBUG
			fprintf (stderr,"Resending packet from recv queue seq %i\n",(int)pack->seq);
#endif
			return;
		}
	}
	if (syncing) retransmit (pack->seq);
}

void
RMTPClient::sendRetrans(PacketData *pack,IP *ip)
{
        struct sockaddr_in server;
	
	memset(&server,0,sizeof(server));
	
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=ip->s_addr;
	server.sin_port=htons(COMMAND_PORT);
	pack->type=RETRANSMITTED;
	if (sendto(fd_data,pack,sizeof(PacketData),0,(struct sockaddr *) &server,sizeof(server)) < 0) {
		fprintf(stderr,"sendto\n");
		exit(1);
	}
	pack->type=DATA_PACKET;
}

void
RMTPClient::fillGaps()
{
// useless now ;)
#if 0
	if (window_recv[0].type!=DATA_EOF) {
		for (int i=RCV_WINDOW-1;i>=1;i--) {
			if (window_recv[i].seq==0) {
				memcpy (&window_recv[i], &window_recv[i-1], sizeof(PacketData));
				window_recv[i-1].seq=0;
				if (relay) {
					rwindow_status[i]=rwindow_status[i-1];
					rwindow_status[i-1]=false;
				}	
			}
		}
	}
#endif
}

void
RMTPClient::fillQueue()
{
        int n;
	PacketData recvdData;
	struct list_head *scan;
	PacketList *packet;
	
#if 0
	fillGaps();
	for (i=RCV_WINDOW-1;i>=0;i--) {
		if (window_recv[i].seq==0) {
			idx=i;
			break;
		}
	}
	if (idx==-1) {
		//fprintf (stderr, "No space in window rcv, waiting a transmit...\n");
		return;
	}
#endif
	n=read(fd_data, &recvdData, sizeof(PacketData));
	
	if (n==-1) {
		fprintf (stderr, "Error while reading from fd_data\n");
	} else if (n!=0) {
		list_for_each (scan, &list_recv) {
			PacketList *p=list_entry(scan,PacketList,l);
			if (recvdData.seq==(p->pkt).seq) return; //already received
		}
		if (crc16(recvdData.buff,recvdData.len)!=recvdData.crc) {
			fprintf (stderr,"Wrong CRC for pack seq %i recorded crc is : %i and calculated crc is %i. Packet will be resent\n",(int)recvdData.seq,recvdData.crc,crc16(recvdData.buff,recvdData.len));
			retransmit(recvdData.seq);
			return;
		}
		nb_packet_received++;
		packet=new PacketList();
		memcpy (&packet->pkt, &recvdData, sizeof(PacketData));
		packet->status=false;
		list_add(&packet->l,&list_recv);
		
#if 0
		for (i=0;i<RCV_WINDOW;i++) { 
			if (window_recv[i].seq==recvdData.seq) return;
		}
		if (crc16(window_recv[i].buff,window_recv[i].len)!=window_recv[i].crc) {
			fprintf (stderr,"Wrong CRC for pack seq %i\n",recvdData.seq);
			retransmit(recvdData.seq);
			return;
		}
		nb_packet_received++; 
		memcpy(&window_recv[idx], &recvdData,sizeof(PacketData)); 
		if (relay) {
			rwindow_status[idx]=0;
		}	
#endif
	}
}


void
RMTPClient::sendAck()
{
        PacketCommand p;
        struct sockaddr_in sin;
		 
        p.type=ACK;
        p.seq=last_seq_completed;
        p.src.s_addr=my_ip.s_addr;
					 
        sin.sin_port=htons(COMMAND_PORT);
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=parent.s_addr;
										 
	if (sendto(fd_command,&p,sizeof(PacketCommand),0,(struct sockaddr *) &sin,sizeof (struct sockaddr_in)) < 0) {
		fprintf(stderr,"error in sendto\n");
	}
}

void
RMTPClient::erasePacksInQueue()
{
	int j;
	bool acked;
	struct list_head *scan;
	PacketList *p;
	
	if (!relay || nb_clients==0) {
		list_for_each (scan,&list_recv) {
			PacketList *p=list_entry(scan,PacketList,l);
			if ((p->pkt).seq<=last_seq_completed) {
				list_del(&p->l);
				delete p;
			}
		}
		return;
	}
	
	list_for_each(scan,&list_recv) {
		acked=true;
		p=list_entry(scan,PacketList,l);
		for (j=0;j<nb_clients;j++) {
			if (ack_list[j]<(p->pkt).seq) {
				acked=false;
				break;
			}
		}
		
		if ( (p->status || last_seq_completed >= (p->pkt).seq ) && acked ) {
			list_del(&p->l); 
			delete p;
		}
	}
#if 0
	if (!relay || nb_clients==0) {
		for (i=0;i<RCV_WINDOW;i++) {
			if (window_recv[i].seq<=last_seq_completed) window_recv[i].seq=0;
		}
		return;
	}
	
	for (i=0;i<RCV_WINDOW;i++) {
		acked=true;
		for (j=0;j<nb_clients;j++) {
			if (ack_list[j]<window_recv[i].seq) {
				acked=false;
				break;
			}
		}
		
		if ( (rwindow_status[i]==true || last_seq_completed >= window_recv[i].seq) && acked ) { 
			//fprintf (stderr, "*Deleting packet seq %i in window_send[%i]\n",(int)window_recv[i].seq,i);
			window_recv[i].seq=0;
			rwindow_status[i]=false;
		}
	}
#endif
	
	list_for_each (scan, &list) {
		acked=true;
		p=list_entry(scan,PacketList,l);
		for (j=0;j<nb_clients;j++) {
			if (ack_list[j]<(p->pkt).seq) {
				acked=false;
				break; 
			}
		}
		if (acked) {
#ifdef DEBUG
			fprintf (stderr,"Erased %i in retr queue\n",(int)(p->pkt).seq);
#endif
			list_del(&p->l);
			delete p;
		}
	}

}

void
RMTPClient::transmitToDest()
{
	struct list_head *scan;
	PacketList *p=NULL;
	bool found=false;
	int n;
	unsigned long max=0;

	if (list_empty(&list_recv)) return;
	
	list_for_each (scan,&list_recv) {
		p=list_entry(scan,PacketList,l);
		max=MAX((p->pkt).seq,max);
		if ( (p->pkt).seq==last_seq_completed+1) {
			found=true;
			break;
		}
	}

	if (!found && max > last_seq_completed) {
		if (wait < 0) {
			wait=0;
			retransmit(last_seq_completed+1);
			wait++;
		} else if (wait==100000) {
			wait=-1;
		} else wait++;
		return;
	} else if (found) {
		wait=-1;
		if (last_seq_completed % ack_inter == 0) sendAck();
		if ((p->pkt).type==DATA_PACKET) {
			n=writeDest((p->pkt).buff,(p->pkt).len);
			if (n==-1) fprintf (stderr,"Write error to fd_dest\n");
			if (relay) p->status=true;
			else {
				list_del(&p->l);
				delete p;
			}
			last_seq_completed++;			
		} else if ((p->pkt).type==DATA_EOF) {
			last_seq_completed++;
			sendAck();
			receiving=false;
			client_status=RMTP_TRANSFER_SUCCESS;
		}
	} else if (syncing && last_seq_completed < syncseq) {
		 retransmit (last_seq_completed+1);
	}
	
	
#if 0
	int i,n;
	int next=RCV_WINDOW-1; //we know there is something (not null) at this place
	
	for (i=RCV_WINDOW-1;i>=0;i--) {
		if (window_recv[i].seq==last_seq_completed+1) {
			next=i;
			break;
		}
		else if (relay && window_recv[i].seq!=0 && rwindow_status[i]==0 && window_recv[i].seq<window_recv[next].seq) next=i;
		else if (!relay && window_recv[i].seq!=0 && window_recv[i].seq<window_recv[next].seq) next=i;
	}
	
	if (window_recv[next].seq>last_seq_completed+1) {
		if (wait < 0) {
			wait=0; //start the counter
			retransmit(last_seq_completed+1);		
			wait++;
		} else if (wait==10000) {
			wait=-1;
		} else {
			wait++;
		}	
		return;
	} else if (window_recv[i].seq==last_seq_completed+1){ // ok write it !
		wait=-1; // counter is disabled
		if (last_seq_completed % ACK_INTERVAL == 0) sendAck();
		if (window_recv[next].type==DATA_PACKET) {
			n=write(fd_dest,&window_recv[next].buff,window_recv[next].len);
			if (n==-1) fprintf (stderr,"Write error to fd_dest\n");
			if (relay) rwindow_status[next]=true;
			else window_recv[next].seq=0; // sent and deleted :)
			last_seq_completed++;
		} else if (window_recv[next].type==DATA_EOF) {
			fprintf (stderr, "The end, closing sockets\n");
			last_seq_completed++;
			say(CLIENT_LEAVE,&parent);
			sendAck();
			if (!relay) {
				close (fd_dest);
				close (fd_data);
				receiving=false;
				exit(0);
			}
		}
	} else if (syncing && last_seq_completed < syncseq) {
		retransmit (last_seq_completed+1); // if i'm here it means i've done nothing, so force actions
	}
#endif
}

void
RMTPClient::initQueue()
{
	int i;
#if 0
	for (i=0;i<RCV_WINDOW;i++) window_recv[i].seq=0;
#endif
	for (i=0; i < RMTP_MAX_CLIENTS ; i++){
		list_sync[i]=0;
		liste_client[i].receive_ready=false;
		liste_client[i].id=0;
		liste_client[i].ip_client.s_addr=0;
	}
}

void
RMTPClient::listQueue()
{
#if 0
	int i;
	for (i=0;i<RCV_WINDOW;i++) fprintf (stderr, "i: %i seq: %i\t",i,(int)window_recv[i].seq);
#endif
	fprintf (stderr,"\n");
}

int
RMTPClient::getStatus()
{
	return client_status;
}

/*****************************************************************************************/
unsigned short crc16(char *data_p, unsigned short length)
{
	unsigned char i;
	unsigned int data;
	unsigned int crc = 0xffff;
	
	if (length == 0)
		return (~crc);
	do
	{
		for (i=0, data=(unsigned int)0xff & *data_p++;i<8;i++, data >>= 1)
		{
			if ((crc & 0x0001) ^ (data & 0x0001))
			crc = (crc >> 1) ^ POLY;
			else  crc >>= 1;
		}
	} while (--length);

	crc = ~crc;
	data = crc;
	crc = (crc << 8) | (data >> 8 & 0xff);
	return (crc);
}

int
get_ip (char *device,IP *ip)
{
	struct sockaddr *sa;
	struct sockaddr_in *sin;
	struct ifreq ifr;
	int fd;

	fd = socket (AF_INET, SOCK_STREAM, 0);

	strncpy (ifr.ifr_name, device, IFNAMSIZ);

	if (ioctl (fd, SIOCGIFADDR, &ifr) == -1)
	{
		fprintf (stderr,"get_ip: ioctl failed\n");
		return -1;
	}
	
	sa = (struct sockaddr *) &(ifr.ifr_addr);
	if (sa->sa_family == AF_INET)
	{
		sin = (struct sockaddr_in *) sa;
		ip->s_addr=sin->sin_addr.s_addr;
	}
	close (fd);
	return 0;
}


