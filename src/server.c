#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "../include/server.h"

int get_listener_socket(const char *port){
    int status;
    struct addrinfo hints, *servinfo, *ptr;
    int sockfd;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((status = getaddrinfo(NULL, port, &hints, &servinfo))!=0){
        fprintf(stderr,"Error en getaddrinfo %s\n", gai_strerror(status));
        return -1;
    }

    for(ptr = servinfo; ptr!= NULL; ptr = ptr -> ai_next){
        if((sockfd = socket(ptr ->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1){
            perror("Error al obtener sockfd");
            continue;
        }

        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            perror("Error en setsockopt");
            close(sockfd);
            continue;
        }

        if(bind(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1){
            perror("Error en bind");
            close(sockfd);
            continue;
        }

        break;
    }
    
    freeaddrinfo(servinfo);

    if (listen(sockfd, BACKLOG) == -1){
        perror("Error en listen");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}
