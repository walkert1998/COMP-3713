#ifndef _MONITOR_H_
#define _MONITOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>

typedef struct status_struct
{
    pthread_mutex_t mutex;  /* Lock for the data structure */

    int total;
    int successful;
    int forbidden;
    int not_found;
    int other_fails;
    void * data;
    size_t size;
} status_s; 
status_s * setup_status(size_t size);
int inc_total(status_s *st);
int inc_successful(status_s *st);
int inc_forbidden(status_s *st);
int inc_not_found(status_s *st);
int inc_other_fails(status_s *st);

#endif