#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "server.h"
#define MAX_EVENTS 20

int main(void) {
    int fd, epfd, ready, i;
    struct epoll_event ev;
    struct epoll_event evlist[MAX_EVENTS];

    fd = get_listener_socket("8000");
    if(fd == -1){
        fprintf(stderr, "Errror con get_listener_socket");
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

                ev.events = EPOLLIN;
                ev.data.fd = client_fd;

                if(epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) == -1){
                    perror("Error al agregar client_fd a epoll");
                    close(client_fd);
                }else{
                    printf("Conexión aceptada\n");
                }
            }

            else{
                int client_fd = evlist[i].data.fd;
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));

                ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer)-1);

                if(bytes_read<=0){
                    printf("Cliente desconectado\n");
                    close(client_fd);
                }else{
                    buffer[bytes_read] = '\0';
                    printf("Solicitud recibida\n");



                    const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hola desde MiniHTTPd</h1";
                    write(client_fd, response, strlen(response));

                    close(client_fd);
                }
            }
        }
    }

    close(fd);
    close(epfd);
    return 0;
    
}