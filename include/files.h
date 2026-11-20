#ifndef FILES_H
#define FILES_H

#include <limits.h>
#include <sys/types.h>

struct document_root{
    char path[PATH_MAX];
    int fd;
};

int abrir_root_publica(struct document_root *root, const char *path);

int abrir_archivo(struct document_root *root, const char *path, int *fd, off_t *size, const char **type);

#endif