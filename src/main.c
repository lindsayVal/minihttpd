#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "server.h"

int main(void) {
    int fd, epfd;
    struct epoll_event ev;

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

    //prueba temporal
    printf("Socket escuchando en puerto 8000\n");
    getchar();

    close(fd);
    close(epfd);
    return 0;
    
}