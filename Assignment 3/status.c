/*
 *	File: status.c
 *	Author: Thomas Walker
 *	Date: Feb 17, 2018
 *	Version: 1.0
 *	Purpose: Creates status object that can track response types.
 *
 *  Notes: This program is built off the monitor example from duanes sample code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include "status.h"
    
status_s * setup_status(size_t size)
{
    pthread_mutexattr_t mutex_attrs;
    size_t memory_size;
    void * allocated;
    status_s *st;

    /* Find the size we need for memory.  A stack header, an array of sizes,
     * and an array of buffers for the stack element contents
     */
    memory_size = sizeof(st) + size * sizeof(size_t);

    /* Allocate the memory.  Let's make it shared so all threads AND 
     * children (and childrens' threads) can access this
     */
    allocated = mmap(NULL, memory_size, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (allocated == NULL)
        return NULL;

    // Initialize all attributes of the status struct
    st = (status_s *) allocated;
    st->size = memory_size;
    st->total = 0;
    st->successful = 0;
    st->forbidden = 0;
    st->not_found = 0;
    st->other_fails = 0;

    /* Prepare attributes for a mutex that can be used by multiple processes */
    pthread_mutexattr_init(&mutex_attrs);
    pthread_mutexattr_setpshared(&mutex_attrs, PTHREAD_PROCESS_SHARED);

    /* Initialize the mutex for the lock on the stack */
    if (pthread_mutex_init(&(st->mutex), &mutex_attrs) != 0)
    {
        munmap(allocated, memory_size);
        return NULL;
    }

    st->data = allocated + sizeof(st) * sizeof(size_t);

    return st;
}

// increments and returns the number of total requests made
int increment_status(status_s * s, int var)
{
    if (s == NULL)
    {
        fprintf(stderr, "Error tracking status.\n");
        return EXIT_FAILURE;
    }
        
    if (pthread_mutex_lock(&(s->mutex)) != 0)
    {
        return EXIT_FAILURE;
    }
    
    else
    {
        
        switch(var)
        {
            case 0:
                s->total++;
                break;
            case 1:
                s->successful++;
                break;
            case 2:
                s->forbidden++;
                break;
            case 3:
                s->not_found++;
                break;
            case 4:
                s->other_fails++;
                break;
            default:
                break;
        }
    }
    
    if (pthread_mutex_unlock(&(s->mutex)) != 0)
    {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;    
}