#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "files.h"
#include "mime.h"

static int abrir_path_rel(int root_fd, char *relative){
    char *segment;
    int dir_fd = root_fd;
    int fd, error, flags;

    while((segment = strsep(&relative, "/")) != NULL){
        flags = O_RDONLY | O_NOFOLLOW | O_CLOEXEC;
        if(relative==NULL){
            flags = flags |O_NONBLOCK;
        }else{
            flags = flags| O_DIRECTORY;
        }
        fd = openat(dir_fd, segment, flags);
        error = errno;
        if(dir_fd != root_fd){
            close(dir_fd);
        }
        if(fd == -1 || relative == NULL){
            errno =error;
            return fd;
        }
        dir_fd = fd;
    }
    errno = ENOENT;
    return -1;
}

int abrir_root_publica(struct document_root *root, char *path){
    if(realpath(path, root->path)==NULL){
        return -1;
    }

    root->fd = open(root->path, O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
    if(root->fd == -1){
        return -1;
    }
    return 0;
}

int abrir_archivo(struct document_root *root, char *path, int *fd, off_t *size, const char **type){
    char candidate[PATH_MAX];
    char resolved[PATH_MAX];
    char *relative;
    char *resource = path;
    struct stat info;
    size_t path_length;
    size_t root_length;
    int length;
    int file_fd = -1;

    path_length = strcspn(path, "?");
    if(path[0] != '/'||path_length>=sizeof(candidate)|| memchr(path, '\\', path_lenght)!= NULL){
        return 400;
    }

    if(memmem(path, path_length, "/../", 4)!= NULL||(path_lenght>=3&&memcmp(path+path_length-3, "/..", 3)==0)){
        return 403;
    }
    if(path_length == 1){
        resource = "/index.html";
        path_length = strlen(resource);
    }
    length = snprintf(candidate, sizeof(candidate), "%s%.*s",root->path, (int)path_length, resource);
    if(length <0||(size_t)length >= sizeof(candidate)){
        return 400;
    }
    if(realpath(candidate, resolved) != NULL){
        root_length = strlen(root->path);
        if(root_length==1&&root->path[0] == '/'){
            relative = resolved + 1;
        }else{
            if(strncmp(resolved, root->path, root_length) != 0 || resolved[root_length] != '/'){
                return 403;
            }
            relative = resolved + root_length + 1;
        }
        if(*relative == '\0'){
            return 403;
        }
        *type = mime_type(resolved);
        file_fd = abrir_path_rel(root->fd, relative);
    }
    if(file_fd==-1){
        int error = errno;
        if(error == ENOENT||error == ENOTDIR){
            return 404;
        }else if(error == EACCES||error == EPERM ||error == ELOOP){
            return 403;
        }else if(error ==ENAMETOOLONG){
            return 400;
        }else{
            return 500;
        }
    }
    if(fstat(file_fd, &info) == -1){
        close(file_fd);
        return 500;
    }
    if(!S_ISREG(info.st_mode)){
        close(file_fd);
        return 403;
    }
    *fd = file_fd;
    *size = info.st_size;
    return 200;
}


