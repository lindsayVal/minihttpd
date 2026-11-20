#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <errno.h>
#include <sys/socket.h>
#include <signal.h>

#include "server.h"
#include "http.h"
#define MAX_EVENTS 20

static void liberar_conexion(struct connection *cnct){
    if(cnct->response.file_fd != -1){
        close(cnct->response.file_fd);
    }
    close(cnct->fd);
    free(cnct);
}

static int registrar_events(int epfd, int op, int fd, struct connection *cnct, unsigned int events){
    struct epoll_event ev ={0};
    ev.data.ptr = cnct;
    ev.events = events;
    return epoll_ctl(epfd, op, fd, &ev);
}

static void acept_connections(int epfd, int fd){
    int client_fd;
    struct connection *cnct;
    while(1){
        client_fd = accept4(fd, NULL,NULL, SOCK_NONBLOCK|SOCK_CLOEXEC);
        if(client_fd == -1){
            if(errno == EINTR || errno == ECONNABORTED){
                continue;}
            if(errno != EAGAIN && errno != EWOULDBLOCK){
                perror("Error en accept4");}
            return;
        }
        cnct = calloc(1, sizeof(*cnct));
        if(cnct == NULL){
            perror("Error al asignar memoria");
            close(client_fd);
            return;
        }
        cnct->fd = client_fd;
        cnct->response.file_fd=-1;

        if(registrar_events(epfd, EPOLL_CTL_ADD, client_fd, cnct, EPOLLIN) == -1){
            perror("Error al agregar cliente a epoll");
            liberar_conexion(cnct);
        }
    
    }
}

static int preparar_resp(struct connection *cnct){
    struct http_request req;
    struct http_response *res = &cnct->response;
    char *end;
    const char *type = "text/plain";
    size_t length;
    size_t index;
    int status;

    end = memmem(cnct->buffer, cnct->bytes_read, "\r\n\r\n",4);
    if(end == NULL){
        if(cnct->bytes_read < sizeof(cnct->buffer)){
            return 0;
        }

        status = 400;
        cnct->bytes_read = 0;
    }else{
        length = (size_t)(end - cnct->buffer) + 4;
        status = analizar_sol(cnct->buffer, length, &req);
        cnct->bytes_read = cnct->bytes_read - length;
        for(index = 0; index < cnct->bytes_read; index = index +1){
            cnct->buffer[index]= cnct->buffer[length + index];
        }

        if(status == 200){
            res->keep_alive = req.keep_alive;
            res->file_fd = -1;
            res->file_size = 0;
        }
    }

    if(status!=200){
        type = "text/plain";
        if(status==400||status == 405){
            res->keep_alive=0;
        }
    }
    if(construir_respuesta(res, status, type) == -1){
        return -1;
    }
    return 1;
}

static int enviar_resp(struct connection *cnct){
    struct http_response *res = &cnct->response;
    ssize_t bytes;
    off_t remaining;

    while(res->header_sent < res->header_length){
        bytes=write(cnct->fd, res->header+res->header_sent, res->header_length - res->header_sent);

        if(bytes>0){
            res->header_sent = res->header_sent+(size_t)bytes;
        }else if(bytes<0&&errno == EINTR){
            continue;
        }else{
            if(bytes < 0 &&(errno==EAGAIN||errno==EWOULDBLOCK)){
                return 0;
            }
            return -1;
        }
    }

    while(res->file_sent<res->file_size){
        remaining = res->file_size - res->file_sent;
        bytes = sendfile(cnct->fd, res->file_fd, &res->file_sent, (size_t)remaining);

        if(bytes>0 || (bytes<0&&errno == EINTR)){
            continue;
        }if(bytes<0 && (errno==EAGAIN||errno == EWOULDBLOCK)){
            return 0;
        }
        return -1;
    }

    return 1;
}

static int atender_connection(int epfd, struct connection *cnct){
    struct http_response *res = &cnct->response;
    int status;
    ssize_t bytes;

    while(1){
        if(res->header_length !=0){
            status = enviar_resp(cnct);
            if(status == -1||(status==1&&!res->keep_alive)){
                return -1;
            }
            if(status == 0){
                return registrar_events(epfd, EPOLL_CTL_MOD, cnct->fd, cnct, EPOLLOUT);
            }
            if(res->file_fd!= -1){
                close(res->file_fd);
            }
            memset(res, 0, sizeof(*res));
            res->file_fd=-1;
        }
        status = preparar_resp(cnct);
        if(status == -1){
            return -1;
        }
        if(status == 1){
            continue;
        }
        bytes = read(cnct->fd, cnct->buffer + cnct->bytes_read, sizeof(cnct->buffer)-cnct->bytes_read);
        if(bytes>0){
            cnct->bytes_read = cnct->bytes_read + (size_t)bytes;
        }else if(bytes<0 && errno == EINTR){
            continue;
        }else if(bytes<0 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            return registrar_events(epfd, EPOLL_CTL_MOD, cnct->fd, cnct, EPOLLIN);
        }else{ return -1;}
    }
}


int main(void) {
    int fd, epfd, ready, i;
    int result = EXIT_SUCCESS;
    struct epoll_event evlist[MAX_EVENTS];
    struct connection *cnct;

    if(signal(SIGPIPE, SIG_IGN)==SIG_ERR){
        perror("Error al configurar SIGPIPE");
        return EXIT_FAILURE;
    }

    fd = get_listener_socket(PORT);
    if(fd == -1){
        fprintf(stderr, "Error con get_listener_socket\n");
        return EXIT_FAILURE;
    }

    epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd == -1){
        perror("Error en epoll_create1");
        close(fd);
        return EXIT_FAILURE;
    }

    if(registrar_events(epfd, EPOLL_CTL_ADD, fd, NULL, EPOLLIN)==-1){
        perror("Error al agregar el socket de escucha a epoll");
        close(fd);
        close(epfd);
        return EXIT_FAILURE;
    }

    printf("Socket escuchando en puerto %s\n", PORT);
    
    while (1) {
        ready = epoll_wait(epfd, evlist, MAX_EVENTS, -1);
        if(ready == -1){
            if(errno == EINTR){
                continue;
        }
        perror("Error en epoll_wait");
        result = EXIT_FAILURE;
        break;
        }   

        for(i = 0; i<ready;i++){
            cnct = evlist[i].data.ptr;
            if(cnct == NULL){
                acept_connections(epfd, fd);
            }else if((evlist[i].events & EPOLLERR) != 0 || atender_connection(epfd, cnct) == -1){
                liberar_conexion(cnct);    
            }
        }
    }

    close(fd);
    close(epfd);
    return result;

}