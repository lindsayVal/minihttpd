#define _GNU_SOURCE
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "http.h"

static int es_caracter_de_token(unsigned char ch){
    return (ch >= '0'&&ch<= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch != '\0' && strchr("!#$%&'*+-.^`_|~", ch) != NULL);
}

static char *separar_linea(char **next_line){
    char *line = *next_line;
    char *line_end = strstr(line, "\r\n");

    if(line_end == NULL){
        return NULL;
    }

    *line_end = '\0';
    *next_line = line_end + 2;
    return line;
}

static char *recortar_espacio(char *value){
    size_t length;

    value = value + strspn(value, " \t");
    length = strlen(value);
    while(length>0&&(value[length-1]== ' '|| value[length -1] == '\t')){
        length = length -1;
    }
    value[length] = '\0';
    return value;
}

static int solicita_cierre(char *value){
    char *token;

    while((token = strsep(&value, ","))!=NULL){
        if(strcasecmp(recortar_espacio(token),"close")==0){
            return 1;
        }
    }

    return 0;
}

static int analizar_linea_solicitud(char *line, struct http_request *req){
    char *method = strsep(&line, " ");
    char *path = strsep(&line, " ");
    char *version = line;
    char *ptr;
    size_t method_length;
    size_t path_length;
    int is_get;

    if(*method == '\0' || path == NULL || *path == '\0' ||
       version == NULL || strcmp(version, "HTTP/1.1") != 0){
        return 400;
    }

    for(ptr = method; *ptr != '\0'; ptr = ptr + 1){
        if(!es_caracter_de_token((unsigned char)*ptr)){
            return 400;
        }
    }

    is_get = strcmp(method, "GET") == 0;
    path_length = strlen(path);
    if(path_length >= sizeof(req->path) || (is_get && path[0] != '/')){
        return 400;
    }
    for(ptr = path; *ptr != '\0'; ptr = ptr + 1){
        if((unsigned char)*ptr <= 32 || (unsigned char)*ptr >= 127 || *ptr == '#'){
            return 400;
        }
    }

    method_length = strlen(method);
    if(method_length < sizeof(req->method)){
        memcpy(req->method, method, method_length + 1);
    }
    memcpy(req->path, path, path_length + 1);
    if(is_get){
        return 200;
    }
    return 405;
}

int analizar_sol(char *raw_request,size_t length,struct http_request *req){
    char request[MAX_REQUEST_SIZE + 1];
    char *next_line = request;
    char *line;
    char *value;
    char *ptr;
    int has_host = 0;
    int has_agent = 0;
    int has_length = 0;
    int has_body = 0;
    int status;

    if(raw_request == NULL || req == NULL || length > MAX_REQUEST_SIZE ||
       memchr(raw_request, '\0', length) != NULL){
        return 400;
    }

    memset(req, 0, sizeof(*req));
    memcpy(request, raw_request, length);
    request[length] = '\0';

    line = separar_linea(&next_line);
    if(line == NULL){
        return 400;
    }
    status = analizar_linea_solicitud(line, req);
    if(status == 400){
        return status;
    }
    req->keep_alive = status == 200;

    while((line = separar_linea(&next_line))!=NULL&& *line != '\0'){
        value = strchr(line, ':');
        if(value == NULL || value == line){
            return 400;
        }
        *value = '\0';
        value = value + 1;

        for(ptr = line; *ptr != '\0'; ptr = ptr + 1){
            if(!es_caracter_de_token((unsigned char)*ptr)){
                return 400;
            }
        }
        for(ptr = value; *ptr != '\0'; ptr = ptr + 1){
            if(((unsigned char)*ptr < 32 && *ptr != '\t') || *ptr == 127){
                return 400;
            }
        }
        value = recortar_espacio(value);

        if(strcasecmp(line, "Host") == 0){
            if(has_host || *value == '\0' || strpbrk(value, " \t,/\\@?#") != NULL){
                return 400;
            }
            has_host = 1;
        }else if(strcasecmp(line, "Connection") == 0){
            if(solicita_cierre(value)){
                req->keep_alive = 0;
            }
        }else if(strcasecmp(line, "User-Agent") == 0){
            if(has_agent || *value == '\0'){
                return 400;
            }
            has_agent = 1;
        }else if(strcasecmp(line, "Content-Length") == 0){
            if(has_length || *value == '\0'){
                return 400;
            }
            has_length = 1;
            for(ptr = value; *ptr != '\0'; ptr = ptr + 1){
                if(*ptr < '0' || *ptr > '9'){
                    return 400;
                }
                if(*ptr != '0'){
                    has_body = 1;
                }
            }
        }
    }

    if(line == NULL|| *next_line != '\0'|| !has_host ||(status == 200&&has_body)){
        return 400;
    }
    return status;
}

int construir_respuesta(struct http_response *res,int status, const char *type){
    const char *reason;
    const char *connection = "close";
    const char *allow = "";
    int length;

    if(status == 200){
        reason = "OK";
    }else if(status == 400){
        reason = "Bad Request";
    }else if(status == 403){
        reason = "Forbidden";
    }else if(status == 404){
        reason = "Not Found";
    }else if(status == 405){
        reason = "Method Not Allowed";
        allow = "Allow: GET\r\n";
    }else{
        status = 500;
        reason = "Internal Server Error";
    }
    if(res->keep_alive){
        connection = "keep-alive";
    }

    length = snprintf(res->header, sizeof(res->header),"HTTP/1.1 %d %s\r\nContent-Length: %jd\r\n""Content-Type: %s\r\nConnection: %s\r\n%s\r\n",status, reason, (intmax_t)res->file_size, type, connection, allow);
    if(length < 0||(size_t)length >= sizeof(res->header)){
        return -1;
    }
    res->header_length = (size_t)length;
    return 0;
}