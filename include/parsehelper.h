#ifndef PARSE_HELPER_H
#define PARSE_HELPER_H

#include <stdio.h>
#include <stdbool.h>

//stores simple information about the line and position of the character of a particular line we are on
//also stores the last word letter after the first word we found
typedef struct {
    size_t line;
    size_t pos;
    size_t lastword_pos; //points to the last letter after a word

} parse_helper;



//creates a parse_helper struct
//sets line,pos, and lastword_pos to 1
void init_ph(parse_helper* tmp);

//simple fgetc wrapper to increment ph as needed
//lines is increased for every '\n' and pos is set back to zero
//else pos is incremented by 1
//returns -1 on EOF otherwise returns character
int ph_get(FILE* file, parse_helper* ph);

//removes preceding whitespace
//returns last character found or -1 if EOF
int ph_ignorews(FILE* file, parse_helper* ph) ;

//Reads the next string of characters until it encounters a space or EOF
//It will automatically allocate space for buf and set pbufsize to the length of buf including the null terminator
//If either is NULL the result is discarded
//Returns number of bytes read or -1 if it fails (Memory Allocation failure) or encounters EOF
int readbuf_string(FILE* file,char** buf,size_t* pbufsize,parse_helper* ph);

int readbuf_uint(FILE* file,char** buf,size_t* pnum,size_t* bufsize,parse_helper* ph);


#endif
