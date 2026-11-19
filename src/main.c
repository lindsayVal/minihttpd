#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "server.h"
#define MAX_EVENTS 20

void liberar_conexion(int epfd, struct connection *cnct){
    if(cnct == NULL) return;
    epoll_ctl(epfd, EPOLL_CTL_DEL, cnct->fd, NULL);
    close(cnct->fd);
    free(cnct);
}

int main(void) {
    int fd, epfd, ready, i;
    struct epoll_event ev;
    struct epoll_event evlist[MAX_EVENTS];

    fd = get_listener_socket("8000");
    if(fd == -1){
        fprintf(stderr, "Error con get_listener_socket\n");
        exit(EXIT_FAILURE);
    }

    epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd == -1){
        perror("Error en epoll_create1");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ev.data.fd = fd;
    ev.events = EPOLLIN;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1){
        perror("Error al agregar fd a la lista de epfd");
        close(fd);
        close(epfd);
        exit(EXIT_FAILURE);
    }

    printf("Socket escuchando en puerto 8000\n");
    
    while (1) {
        ready = epoll_wait(epfd, evlist, MAX_EVENTS, -1);
        if(ready == -1){
            perror("Error en epoll_wait");
        break;
        }

        for(i = 0; i<ready;i++){
            if(evlist[i].data.fd == fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
                if(client_fd == -1){
                    perror("Error al aceptar conexión");
                    continue;
                }

                int flags = fcntl(client_fd, F_GETFL, 0);
                if(flags == -1||fcntl(client_fd,F_SETFL, flags | O_NONBLOCK) == -1){
                    perror("Error al configurar O_NONBLOCK con fcntl");
                    close(client_fd);
                    continue;
                }

                struct connection *cnct = malloc(sizeof(struct connection));
                if(!cnct){
                    perror("Error al asignar memoria para la conexión");
                    close(client_fd);
                    continue;
                }

                cnct->fd = client_fd;
                cnct->bytes_read = 0;
                memset(cnct->buffer, 0, BUFFER_SIZE);

                ev.events = EPOLLIN;
                ev.data.ptr = cnct;

                if(epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) == -1){
                    perror("Error al agregar client_fd a epoll");
                    free(cnct);
                    close(client_fd);
                }else{
                    printf("Conexión aceptada\n");
                }
            }

            else{ 

                struct connection *cnct = (struct connection*)evlist[i].data.ptr;

                ssize_t bytes = read(cnct->fd, cnct->buffer + cnct->bytes_read, BUFFER_SIZE-cnct->bytes_read-1);

                if(bytes < 0){
                    if(errno == EAGAIN || errno == EWOULDBLOCK){
                        continue;
                    }
                    perror("Error en read");
                    liberar_conexion(epfd, cnct);
                }else if(bytes == 0){
                    printf("Cliente desconectado\n");
                    liberar_conexion(epfd, cnct);
                }else{
                    cnct->bytes_read += bytes;
                    cnct->buffer[cnct->bytes_read] = '\0';

                    if(strstr(cnct->buffer, "\r\n\r\n") != NULL){
                        printf("Solicitud completada\n");
                    
                    const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hola</h1>";
                    write(cnct->fd, response, strlen(response));

                    cnct->bytes_read = 0;
                    memset(cnct->buffer,0,BUFFER_SIZE);
                    }
                }
            }
        }
    }

    close(fd);
    close(epfd);
    return 0;
    
}