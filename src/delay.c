

#include <assert.h>


#include "server.h"
#include "delay.h"
#include <stdio.h>


struct client * first_waiting_client = 0;
struct client * last_waiting_client = 0;


struct timeval last_release_date;

/** delay between clients */
long wait_delay;

void set_wait_delay(long musec)
{
	wait_delay = musec;
	last_release_date.tv_sec = 0;
	last_release_date.tv_usec = 0;
	debug("DELAY = %ld usec\n", musec);
}



int is_time_inf(struct timeval * a, struct timeval * b)
{
	return timercmp(a, b, <=);
}

void inc_time_usec(struct timeval * a, long usec)
{
	long musec = usec + a->tv_usec;
	a->tv_sec += musec / 1000000;
	a->tv_usec = musec % 1000000;
}

void add_to_wait_queue(struct client * client)
{
	if (first_waiting_client) /* queue not empty */
	{
		assert(last_waiting_client);
		last_waiting_client->next_waiting_client = client;
		client->release_date = last_waiting_client->release_date;
		inc_time_usec(&(client->release_date), wait_delay);
		last_waiting_client = client;
	}
	else
	{
		struct timeval rl = last_release_date;
		struct timeval now;
		
		gettimeofday(&now, 0);
		inc_time_usec(&rl, wait_delay);

		/* if queue is empty, maybe we can release at once, 
		 * BUT MAYBE NOT : if we just (less than wait_delay ago) released another client, we have to wait*/
		
		if (is_time_inf(&now, &rl))
			client->release_date = rl;
		else
			client->release_date = now;

		first_waiting_client = last_waiting_client = client;
	}
	client->next_waiting_client = 0;
	client->is_waiting_release = 1;
	debug("adding client to queue, release_date = %d,%d\n", (int)client->release_date.tv_sec, (int)client->release_date.tv_usec);
}

void wipe_from_wait_queue(struct client * client)
{
	struct client * cl = first_waiting_client;

	debug("force removing client of the wait queue\n");
	assert(cl);

	if (cl == client)
	{
		first_waiting_client = client->next_waiting_client;
		cl = 0;
	}
	else
	{
		while ((cl) && (cl->next_waiting_client != client))
			cl = cl->next_waiting_client;

		assert(cl);
		cl->next_waiting_client = client->next_waiting_client;
	}
	if (last_waiting_client == client) last_waiting_client = cl;

	client->is_waiting_release = 0;
}
		

struct client * try_to_release_client()
{
	struct timeval now;
	struct client * client;
//	debug("try_to_release_client\n");
	if (!first_waiting_client) return 0; /* queue empty, nobody to release */
	gettimeofday(&now, 0);
	debug("try_to_release_client : \n");
	debug("not empty, date = %d,%d\n", (int)now.tv_sec, (int)now.tv_usec);
	debug("    release_date = %d,%d\n", (int)first_waiting_client->release_date.tv_sec, (int)first_waiting_client->release_date.tv_usec);
	if (is_time_inf(&(first_waiting_client->release_date), &now))
	{
		client = first_waiting_client;
		client->is_waiting_release = 0;

		last_release_date = client->release_date;

		if (last_waiting_client == first_waiting_client)
		{
			first_waiting_client = last_waiting_client = 0;
		}
		else
			first_waiting_client = first_waiting_client->next_waiting_client;
		
		client->next_waiting_client = 0; /* not really useful, but I don't like to have non-existant objects referred */
		
		debug("Releasing client\n");
		return client;
	}
	debug("too early\n");
	return 0; /* too early for release of first client */
}

