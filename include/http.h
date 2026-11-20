#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <sys/types.h>

#define MAX_REQUEST_SIZE 4096

struct http_request{
    char method[16];
    char path[256];
    int keep_alive;
};

struct http_response{
    char header[512];
    size_t header_length;
    size_t header_sent;
    int file_fd;
    off_t file_size;
    off_t file_sent;
    int keep_alive;
};

int analizar_sol(char *raw_request, size_t length,struct http_request *req );

int construir_respuesta(struct http_response *res, int status, const char *type);

#endif