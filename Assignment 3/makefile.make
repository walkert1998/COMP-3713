CFLAGS = -Wall -Wextra -std=gnu11

server:   server.c
		gcc $(CFLAGS) -pthread socket_utils.c parse_request.c status.c server.c -o server -lpthread
		
clean:
		rm server