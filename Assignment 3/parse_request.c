/*
 *	File: parse_request.c
 *	Author: Thomas Walker
 *	Date: Feb 17, 2018
 *	Version: 1.0
 *	Purpose: Reads in HTTP request and makes sure its in the proper format.
 *
 *  Notes: This was separated out to avoid cluttering up the already
 *         over-saturated server.c file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
parse_request(char * string, int fd, size_t size)
{
    FILE * file = fdopen(fd, "r");
    char title[size];
    char value[size];
    char * request;
    size_t length;

    if (file == NULL)
        return -1;

    request = NULL;
    length = 0;


    // Scan first line of request
    if (getline(&request, &length, file) != -1)
        if (sscanf(request, "GET %[^ \t\n\t\f] HTTP/1.1\r\n", string) == 0)
            return -2;
            
    free(request);
    request = NULL;
    length = 0;
    while (getline(&request, &length, file) != -1)
    {
        // Check for end of input
        if (strcmp(request, "\r\n") == 0)
            break;
        // Ensure correct format for title:value
        if (sscanf(request, " %[^:]: %s\r\n", title, value) != 2)
            break;
        free(request);
        request = NULL;
        length = 0;
    }
    free(request);

    if (fclose(file) != 0)
        return -3;

    return 0;
}
