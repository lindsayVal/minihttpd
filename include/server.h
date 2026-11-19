#define _GNU_SOURCE
#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define PORT "8000"
#define BACKLOG 10
#define BUFFER_SIZE 4096

struct connection {
    int fd;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
};

int get_listener_socket(const char *port);


#endif