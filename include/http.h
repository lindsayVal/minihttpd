#ifndef HTTP_H
#define HTTP_H

struct http_request{
    char method[16];
    char path[256];
    int keep_alive;
};

int parse_request(const char *raw_request, struct http_request *req);

#endif