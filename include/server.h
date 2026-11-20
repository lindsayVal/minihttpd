#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>
#include "http.h"

#define PORT "8000"
#define BACKLOG 10

struct connection {
    int fd;
    char buffer[MAX_REQUEST_SIZE];
    size_t bytes_read;
    struct http_response response;
};

int get_listener_socket(const char *port);


#endif