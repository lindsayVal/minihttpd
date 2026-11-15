#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define PORT "8000"
#define BACKLOG 10

int get_listener_socket(const char *port);

void *get_addr(struct sockaddr *sa);

#endif