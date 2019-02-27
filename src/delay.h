
#ifndef DELAY_H_INC
#define DELAY_H_INC


#include "server.h"


void set_wait_delay(long musec);
void inc_time_usec(struct timeval * a, long usec);
int is_time_inf(struct timeval * a, struct timeval * b);
void inc_time_usec(struct timeval * a, long usec);
void add_to_wait_queue(struct client * client);
struct client * try_to_release_client();
void wipe_from_wait_queue(struct client * client);


#endif

