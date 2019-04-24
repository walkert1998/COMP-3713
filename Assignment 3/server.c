/*
 *	File: server.c
 *	Author: Thomas Walker
 *	Date: Feb 17, 2018
 *	Version: 1.0
 *	Purpose: Creates a server that will return HTTP responses to client requests.
 *
 *  Notes: This program and related files takes a LOT of code from Duanes
 *         example programs.
 */
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
#include "parse_request.h"
#include "status.h"


#define BUFFER 1000
/* flag to tell if an interrupt signal has been sent */
static int is_interrupted = 0;

/* Function to set the is_interrupted flag.  
 * This should be called when an interrupt signal is sent 
 */
void sigint_received(int signum)
{
    is_interrupted = 1;
}

/*
 * Name:        log_error
 * Purpose:     Prints error messages to log file
 * Arguments:   Two char * inputs, one for the log file name, and one for the
 *              error message to be output.
 * Output:      error messages to either stderr or to the log file.
 * Modifies:    None.
 * Returns:     None.
 * Assumptions: None.
 * Bugs:        None.
 * Notes:       None.
 */
void log_error(char * log_path, char * msg)
{
    FILE * file = fopen(log_path, "a");
    if (file == NULL)
    {
        fprintf(stderr, "Log file %s could not be opened.\n", log_path);
        exit(1);
    }
    fprintf(file, "Error: %s\n", msg);
    fclose(file);
}

/*
 * Name:        log_response
 * Purpose:     Prints response info messages to log file
 * Arguments:   Two char * inputs, one for the log file name, and one for the
 *              base directory path. One FILE input for the log file and an int
 *              for the response code.
 * Output:      error messages to stderr or response info to the log file.
 * Modifies:    None.
 * Returns:     None.
 * Assumptions: None.
 * Bugs:        None.
 * Notes:       None.
 */
void log_response(char * log_path, char * dir, FILE * file, int code)
{
    file = fopen(log_path, "a");
    if (file == NULL)
    {
        fprintf(stderr, "Log file %s could not be opened.\n", log_path);
        exit(1);
    }
    fprintf(file, "Done: %d %s\n", code, dir);
    fclose(file);
}

/*
 * Name:        build_resposne
 * Purpose:     Creates an HTTP response to be sent back to the client
 * Arguments:   Two char * inputs, one for the log file name, and one for the
 *              base directory. And a status_s struct to track the number of
 *              response types.
 * Output:      error messages to stderr or to the log file.
 * Modifies:    None.
 * Returns:     Char *.
 * Assumptions: None.
 * Bugs:        None.
 * Notes:       None.
 */
char * build_response(char * directory, char * log, status_s * st)
{
    struct stat file_info;
    FILE * request_file = NULL;
    FILE * log_file = NULL;
    char * code_text = (char *) malloc(BUFFER * sizeof(char));
    char * format = (char *) malloc(BUFFER * sizeof(char));
    char * response = (char *) malloc(BUFFER * sizeof(char));
    char * content;
    char * line = NULL;
    int response_code = 0;
    int content_length = 0;
    int len = 0;
    // For some reason without this everything related to the log file
    // becomes a complete mess.
    log = "log-file.txt";
    
    format = "HTTP/1.1 %d %s\r\nContent-Length: %d\r\n\r\n%s";
    
    stat(directory, &file_info);
    
    if (access(directory, F_OK | R_OK) == 0 && (S_ISREG(file_info.st_mode)) != 0)
    {
        response_code = 200;
        code_text = "OK";
        content = (char *) malloc(sizeof(char) * file_info.st_size);
        
        request_file = fopen(directory, "r");
        if (request_file == NULL)
        {
            log_file = fopen(log, "a");
            if (log_file == NULL)
            {
                fprintf(stderr, "Log file %s could not be opened.\n", log);
                fclose(request_file);
                fclose(log_file);
                exit(1);
            }
            char * message = "Requested file %s could not be opened.";
            sprintf(line, message, directory);
            log_error(log, line);
        }
        
        else
        {
            len = fread(content, sizeof(char), file_info.st_size + 1,
                request_file);
                
            if (len != file_info.st_size)
            {
                char * message = "Only %d bytes of %d read in.\n";
                sprintf(line, message, len, file_info.st_size);
                log_error(log, line);
            }
            
            if (increment_status(st, 1) != 0)
            {
                log_error(log, "Shared memory variable not incremented properly.\n");
                exit(1);
            }
        }
        fclose(request_file);
    }
    
    else if (access(directory, F_OK) == -1)
    {
        response_code = 404;
        code_text = "File not found";
        content = "<h1>404 File Not Found</h1>";
        
        if (increment_status(st, 3) != 0)
        {
            log_error(log, "Shared memory variable not incremented properly.\n");
            exit(1);
        }
    }
    
    else if (access(directory, F_OK | R_OK) == -1 || S_ISREG(file_info.st_mode) == 0)
    {
        response_code = 403;
        code_text = "Forbidden";
        content = "<h1>403 Forbidden</h1>";
        if (increment_status(st, 2) != 0)
        {
            log_error(log, "Shared memory variable not incremented properly.\n");
            exit(1);
        }
    }
    
    else
    {
        response_code = 500;
        code_text = "Internal Servor Error";
        content = "<h1>500 Internal Servor Error</h1>";
        
        if (increment_status(st, 4) != 0)
        {
            log_error(log, "Shared memory variable not incremented properly.\n");
            exit(1);
        }
    }
    
    content_length = strlen(content);
    sprintf(response, format, response_code, code_text, content_length, content);
    log_response(log, directory, log_file, response_code);
    
    return response;
}

/*
 * Name:        handle_client
 * Purpose:     Interprets requests and sends response
 * Arguments:   Two char * inputs, one for the log file name, and one for the
 *              error message to be output. One int for the file descriptor and
 *              a status_s struct to track number of response types.
 * Output:      error messages to the log file and response to the socket.
 * Modifies:    None.
 * Returns:     None.
 * Assumptions: None.
 * Bugs:        Concatenating the request to the base directory path
 *              causes the log file name to be changed to part of the request.
 * Notes:       None.
 */
static void handle_client(int fd_read, char * base_directory,
                            char * log_file, status_s * s)
{
    char * request = (char *) malloc(BUFFER * sizeof(char));
    char * file_path = (char *) malloc(BUFFER * sizeof(char));
    char * response = (char *) malloc(BUFFER * sizeof(char));
    char * output = (char *) malloc(BUFFER * sizeof(char));
    int status = 0;
    int fd_write = dup(fd_read);
    status = parse_request(request, fd_read, BUFFER);
    if (status == -1)
    {
        log_error(log_file, "File descriptor could not be opened.");
        return;
    }
    
    else if (status == -2)
    {
        log_error(log_file, "File path could not be read.");
        return;
    }
    
    else if (status == -3)
    {
        log_error(log_file, "File could not be closed.");
        return;
    }
    close(fd_read);
    
    if (strcmp(request, "/status") == 0)
    {
        output = "<h1>Status</h1>\r\n<p>Total Requests: %d<br/>\r\n"
                    "200: %d<br/>403: %d<br/>\r\n404: %d<br/>500: %d\r\n</p>";
        sprintf(response, output, s->total, s->successful, s->forbidden,
                   s->not_found, s->other_fails);
        status = write_in_full(fd_write, response, strlen(response));
        if (status == -1)
        {
            log_error(log_file, "Writing to client failed.");
            return;
        }
        increment_status(s, 1);
        return;
    }
    
    file_path = base_directory;
    strcat(file_path, request);
    
    response = build_response(file_path, log_file, s);
    
    if (response == NULL)
        log_error(log_file, "Response is null.");
        
    
    /* Write the response back as a 4-byte integer */
    status = write_in_full(fd_write, response, strlen(response));
    if (status == -1)
    {
        log_error(log_file, "Writing to client failed.");
        return;
    }
}

int
main(int argc, char * argv[])
{
    int port_number;
    int clients = 1;
    char * directory = NULL;
    char * log_file = NULL;
    
    int status = 0;
    int server_socket = 0;
    int sig_status;
    
    status_s * stat = setup_status(10);
    
    pid_t id;
    
    
    if (argc != 4)
    {
        fprintf(stderr, "Exactly 4 arguments required, %d detected.\n", argc);
        
        return EXIT_FAILURE;
    }
    
    directory = argv[1];
    port_number = atoi(argv[2]);
    log_file = argv[3];

    /* Set up interrupt handler */
    struct sigaction sigint_handler;
    
    struct sockaddr_in address;
    
    address.sin_family = AF_INET;
    address.sin_port = htons(port_number);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    

    sigint_handler.sa_handler = sigint_received;
    sigemptyset(&sigint_handler.sa_mask);
    sigint_handler.sa_flags = 0;
    
    sig_status = sigaction(SIGINT, &sigint_handler, NULL);
    if (sig_status != 0)
    {
        perror(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == -1)
    {
        fprintf(stderr, "Failed to create socket.\n");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "socket created: %d\n", server_socket);
    
    status = bind(server_socket, (SA*)&address, sizeof(address));
    if (status == -1)
    {
        fprintf(stderr, "Bind failure.\n");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "bound: %d\n", status);
    
    status = listen(server_socket, 10);
    if (status == -1)
    {
        fprintf(stderr, "Socket could not listen.\n");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "listening: %d\n", status);
    
    // Main loop for reading in clients
    while (1)
    {
        struct sockaddr_in client_address;
        socklen_t address_size;
        
        int client = accept(server_socket, (SA*)&client_address, &address_size);
        
        if (client == -1)
        {
            log_error(log_file,  "Client accept failed.");
            close(client);
            break;
        }
        fprintf(stderr, "Accepted.  Client No: %d.  File No: %d\n", 
                clients, client);
        
        // Create child process to handle multiple clients
        id = fork();
        
        if (id == 0)
        {
            close(server_socket);
            handle_client(client, directory, log_file, stat);
            if (increment_status(stat, 0) != 0)
            {
                log_error(log_file, "Shared memory variable not incremented properly.");
                close(client);
                break;
            }
            close(client);
            exit(0);
        }
        
        else if (id > 0)
        {
            close(client);
            clients++;
        }
        
        // Wait for all children to die off
        if ((id = waitpid(-1, &status, WNOHANG)) == -1)
        {
            log_error(log_file, "wait() error\n");
        }

        if (is_interrupted == 1)
        {
            close(client);
            break;
            //is_interrupted = 0;
        }
        
        // Increment number of clients
        clients++;
    }
    
    close(server_socket);
    return EXIT_SUCCESS;
}