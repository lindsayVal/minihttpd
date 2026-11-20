#include <string.h>
#include "mime.h"

const char *mime_type(const char *path){
    if(path == NULL){
        return "application/octet-stream";
    }

    const char *name = strrchr(path, '/');
    const char *extension; 

    if(name == NULL){
        name = path;
    }else{
        name = name + 1;
    }

    extension = strrchr(name, '.');

    if(extension == NULL || *(extension + 1) == '\0'){
        return "application/octet-stream";
    }

    if(strcmp(extension, ".html") == 0 || strcmp (extension, ".htm") == 0){
        return "text/html";
    }
    
    if(strcmp(extension, ".css") == 0){
        return "text/css";
    }

    if(strcmp(extension, ".js") == 0){
        return "application/javascript";
    }

    if(strcmp(extension, ".png") == 0){
        return "image/png";
    }
    
    if(strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0){
        return "image/jpeg";
    }

    return "application/octet-stream";

}