#include <stdio.h>
#include <string.h>

#include "http.h"

int parse_request(const char *raw_request, struct http_request *req){
    if(raw_request == NULL|| req == NULL){
        return -1;
    }

    memset(req, 0, sizeof(*req));

    if(sscanf(raw_request, "%15s %255s", req->method, req->path)!=2){
        return -1;
    }

    if(strcmp(req->method, "GET")!=0){
        return -1;
    }

    if(strcmp(req->path, "/") == 0){
        snprintf(req->path, sizeof(req->path), "/index.html");
    }

    if(strstr(raw_request, "Connection: close")!=NULL||strstr(raw_request, "connection: close")!= NULL){
        req->keep_alive = 0;
        }else{
            req->keep_alive = 1;
        }

        return 0;
}