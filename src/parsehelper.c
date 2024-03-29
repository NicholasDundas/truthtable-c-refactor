#include "parsehelper.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

void init_ph(parse_helper* tmp) {
    tmp->line = 1;
    tmp->pos = 1;
    tmp->eofhit = NULL;
    tmp->eofparam = NULL;
}

int ph_get(FILE* file, parse_helper* ph) {
    int c = fgetc(file);
    switch(c) {
        case EOF:
            if(ph->eofhit)
                ph->eofhit(ph->eofparam);
            return EOF;
        case '\n':
            ph->line++;
            ph->pos = 1;
            break;
        default:
            ph->pos++;
            if(isalnum(c)) {
                ph->wrd_end = ph->pos;
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
int readbuf_string(FILE* file,char** pbuf,size_t* pbufsize,parse_helper* ph) { 
    const parse_helper start = *ph;
    int c;
    //if we are passed null buffer or buffersize we just automatically allocate our own and delete at the end
    size_t *bufsize = pbufsize ? pbufsize : calloc(1,sizeof(size_t));
    char** buf = pbuf ? pbuf : calloc(1,sizeof(char*));

    int num_read_and_written = 0; //number of characters written to buffer

    //if either failed to allocate then cleanup the memory
    if(!bufsize || !buf) goto NO_MEM_CLEANUP;
    
    if((c = ph_ignorews(file,ph)) == EOF) goto CLEANUP; //clear leading whitespace characters
    parse_helper old_ph = *ph;
    if(*buf == NULL || *bufsize < 4) { //if the buffer is less than 4
        *buf = realloc(*buf,4);
        if(!(*buf)) goto NO_MEM_CLEANUP;
        *bufsize = 4;
    }
    do { //read in character up until whitespace
        (*buf)[num_read_and_written++] = c;
        if(num_read_and_written == *bufsize) {
            *bufsize = (*bufsize << 1) - (*bufsize >> 1);
            *buf = realloc(*buf,*bufsize);
            if(!*buf) goto NO_MEM_CLEANUP;
        }
    } while(!isspace(c = ph_get(file,ph)) && isprint(c));
    (*buf)[num_read_and_written] = '\0';  

    if(*bufsize > num_read_and_written) { //buffer size is bigger than
        *bufsize = num_read_and_written;
        *buf = realloc(*buf, *bufsize);
        if(!*buf) goto NO_MEM_CLEANUP;
    }
    goto CLEANUP;

NO_MEM_CLEANUP:
    c = EOF;
    errno = ENOMEM;
    if(bufsize) *bufsize = 0;
    fprintf(stderr,"ERROR: Could not allocate space for buffer when reading string\nLINE:%zu\nPOS:%zu",start.line,start.pos);

CLEANUP:
    if(!pbufsize && bufsize) free(bufsize);
    if(!pbuf && buf) {
        if(*buf) free(*buf);
        free(buf);
    }
    if(c == '\n') {
        ph->line = old_ph.line;
        ph->line = old_ph.pos+num_read_and_written;
        ungetc(c,file);
    }
    return c == EOF ? EOF : num_read_and_written;

}

int readbuf_uint(FILE* file,char** buf,size_t* pnum,size_t* bufsize,parse_helper* ph) { 
    int c;
    errno = 0;
    if((c = readbuf_string(file,buf,bufsize,ph)) > 0 && (*buf)[0] != '-') {
        char* ptr;
        errno = 0;
        *pnum = strtoull((*buf),&ptr,10);
        if(*ptr != '\0') {
            errno = EINVAL;
        }
        if(errno != ERANGE) {
            return c == EOF ? EOF : 0;
        }
    } else if (*bufsize > 0) {
        fprintf(stderr,"ERROR: Invalid positive integer given\nBUFFER:\"%s\"\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->wrd_end - *bufsize);
    }
    return 1;
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
        fprintf(stderr,"ERROR: Invalid integer given\nBUFFER:\"%s\"\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->wrd_end - *bufsize);
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
        fprintf(stderr,"ERROR: Invalid integer given\nBUFFER:\"%s\"\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->wrd_end - *bufsize);
    }
    return 0;
}
