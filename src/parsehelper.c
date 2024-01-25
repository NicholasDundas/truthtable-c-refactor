#include "parsehelper.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

void init_ph(parse_helper* tmp) {
    tmp->line = 1;
    tmp->pos = 1;
    tmp->lastword_pos = tmp->pos;
}

int ph_get(FILE* file, parse_helper* ph) {
    int c = fgetc(file);
    switch(c) {
        case EOF:
            return EOF;
        case '\n':
            ph->line++;
            ph->pos = 1;
            break;
        default:
            ph->pos++;
            if(isalnum(c)) {
                ph->lastword_pos = ph->pos;
            }
    }
    return c;
}

int ph_ignorews(FILE* file, parse_helper* ph) {
    int c;
    while(isspace(c = ph_get(file,ph))) {}
    return c;
}

//returns bytes read or EOF on error or EOF
int readbuf_string(FILE* file,char** buf,size_t* pbufsize,parse_helper* ph) { 
    int c;
    size_t *bufsize = pbufsize;
    size_t count = 0;

    if(!bufsize) {
        bufsize = malloc(sizeof(size_t));
    }
    if(buf == NULL || *bufsize < 4) {
        *buf = realloc(*buf,4);
        if(!*buf) {
            goto NO_MEM_CLEANUP;
        }
        *bufsize = 4;
    }
    if((c = ph_ignorews(file,ph)) == EOF) {
        goto CLEANUP;
    }
    
    do {
        (*buf)[count++] = c;
        if(count == *bufsize) {
            *bufsize = (*bufsize << 1) - (*bufsize >> 1);
            *buf = realloc(*buf,*bufsize);
            if(!*buf) {
                goto NO_MEM_CLEANUP;
            }
        }
    } while(!isspace(c = ph_get(file,ph)) && isprint(c));
    (*buf)[count++] = '\0';
    if(c != EOF) {
        ph->lastword_pos = ph->pos - count;
    }
    
    if(*bufsize > count) {
        *bufsize = count;
        *buf = realloc(*buf, *bufsize);
        if(!*buf) {
            goto NO_MEM_CLEANUP;
        }
    }

    CLEANUP:
    if(!pbufsize) free(bufsize);
    return c == EOF ? EOF : count;
    
    NO_MEM_CLEANUP:
    errno = ENOMEM;
    *bufsize = 0;
    fprintf(stderr,"ERROR: could not allocate space for buffer when reading string\nLINE:%zu\nPOS:%zu",ph->line,ph->lastword_pos-count);
    if(!pbufsize) free(bufsize);
    return EOF;
}

int readbuf_uint(FILE* file,char** buf,size_t* pnum,size_t* bufsize,parse_helper* ph) { 
    int c;
    errno = 0;
    if((c = readbuf_string(file,buf,bufsize,ph)) != 0 && *bufsize > 0 && (*buf)[0] != '-') {
        char* ptr;
        errno = 0;
        *pnum = strtoull((*buf),&ptr,10);
        if(*ptr != '\0') {
            errno = EINVAL;
        }
        if(errno != ERANGE) {
            return c == EOF ? EOF : 1;
        }
    } else if (*bufsize > 0) {
        fprintf(stderr,"ERROR: Invalid positive integer given\nBUFFER:\"%s\"\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->lastword_pos - *bufsize);
    }
    return 0;
}

int readbuf_long(FILE* file,char** buf,long* pnum,size_t* bufsize,parse_helper* ph) { 
    int c;
    errno = 0;
    if((c = readbuf_string(file,buf,bufsize,ph)) != 0 && *bufsize > 0) {
        char* ptr;
        errno = 0;
        *pnum = strtol((*buf),&ptr,10);
        if(*ptr != '\0') {
            errno = EINVAL;
        } 
        if(errno != ERANGE) {
            return c == EOF ? EOF : 1;
        }
    } else if (*bufsize > 0) {
        fprintf(stderr,"ERROR: Invalid integer given\nBUFFER:\"%s\"\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->lastword_pos - *bufsize);
    }
    return 0;
}

int readbuf_llong(FILE* file,char** buf,long long* pnum,size_t* bufsize,parse_helper* ph) { 
    int c;
    errno = 0;
    if((c = readbuf_string(file,buf,bufsize,ph)) != 0 && *bufsize > 0) {
        char* ptr;
        errno = 0;
        *pnum = strtoll((*buf),&ptr,10);
        if(*ptr != '\0') {
            errno = EINVAL;
        } 
        if(errno != ERANGE) {
            return c == EOF ? EOF : 1;
        }
    } else if (*bufsize > 0) {
        fprintf(stderr,"ERROR: Invalid integer given\nBUFFER:\"%s\"\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->lastword_pos - *bufsize);
    }
    return 0;
}