#include "parsehelper.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

parse_helper* init_ph() {
    parse_helper* tmp = malloc(sizeof(parse_helper));
    tmp->line = 1;
    tmp->pos = 1;
    tmp->lastword_pos = tmp->pos;
    return tmp;
}

void free_ph(parse_helper* ph) {
    free(ph);
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
        case ' ':
            ph->lastword_pos = ph->pos + 1;
            __attribute__ ((fallthrough));
        default:
            ph->pos++;
    }
    return c;
}


int ph_ignorews(FILE* file, parse_helper* ph) {
    int c;
    while(isspace(c = ph_get(file,ph))) {}
    return c;
}


int readbuf_string(FILE* file,char* buf,size_t maxchr,parse_helper* ph) { 
    int c;
    size_t count = 0;

    if((c = ph_ignorews(file,ph)) == EOF) {
        return EOF;
    }
    
    while(count < maxchr && (buf[count++] = (char)c, !isspace(c = ph_get(file,ph)) && isprint(c))) {}
    buf[count] = '\0';

    if (count >= maxchr) {
        fprintf(stderr,"ERROR: Input Buffer overflow (MAX ALLOWED: %zu)\nBUFFER:\"%s\"\nLINE:%zu\nPOSITION:%zu\n",maxchr,buf,ph->line,ph->lastword_pos - count);
        return 0;
    }
    return (c == EOF) ? EOF : 1;
}

//checks if a number is a non-negative digit less than 21 digits
bool is_consect_digit(char* buf) {
    char* ptr = buf;
    while(isdigit(*ptr)) {
        ++ptr; 
    }
    if(*ptr == '\0' && (ptr-buf) < 21) { //21 is the width of characters for size_t....
        return true;
    }
    return false;
}

int readbuf_uint(FILE* file,size_t* pnum,size_t maxchr, parse_helper* ph) { 
    int c;
    char* buf = malloc(maxchr + 1);
    c = readbuf_string(file,buf,maxchr,ph);
    if(is_consect_digit(buf)) {
        char* ptr;
        *pnum = strtoull(buf,&ptr,10);
        if(errno == 0) {
            free(buf);
            return c == EOF ? EOF : 1;
        }
    }
    fprintf(stderr,"ERROR: Invalid integer given\nBUFFER:\"%s\"\nLINE:%zu\nPOSITION:%zu\n",buf,ph->line,ph->lastword_pos - strlen(buf)-2);
    free(buf);
    return 0;
}
