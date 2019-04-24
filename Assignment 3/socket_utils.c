// This is just Duane's code from example 6 but with fixed indentation.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "socket_utils.h"


/* Function: read_in_full
 * Wrapper function for read which reads until EOF, error, or all requested
 * data is read.  Necessary for dealing with signals and other network I/O
 * conditions which may cause read to abnormally abort.
 */
int read_in_full(int fd, void *data, size_t size)
{
    int total_read = 0;
    /* Keep going until all requested bytes have been read... */
    while (total_read < size)
    {
        /* Try reading remaining bytes from current location */
        int n_read = read(fd, data + total_read, size - total_read);
        if (n_read == -1)
        {
            /* If it was interrupted by a signal, try again.  Otherwise error */
            if (errno == EINTR)
                continue;
            else
                return -1;
        }

        /* Update the amount read */
        total_read += n_read;
        /* If end of file, return early */
        if (n_read == 0)
            return total_read;
            
    }
    return total_read;
}



/* Function: write_in_full
 * Wrapper function for write which writes until error, or all requested
 * data is written.  Necessary for dealing with signals and other network I/O
 * conditions which may cause write to abnormally abort.
 */
int write_in_full(int fd, void *data, size_t size)
{
    int total_written = 0;
    /* Keep going until all requested bytes have been written... */
    while (total_written < size)
    {
        /* Try writing remaining bytes from current location */
        int n_written = write(fd, data + total_written, size - total_written);
        if (n_written == -1)
        {
            /* If it was interrupted by a signal, try again.  Otherwise error */
            if (errno == EINTR)
                continue;
            else
                return -1;
        }

        /* Update the amount written */
        total_written += n_written;
    }

    return total_written;
}


