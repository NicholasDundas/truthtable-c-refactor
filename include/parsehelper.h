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
parse_helper* init_ph();

//frees a parse_helper struct
void free_ph(parse_helper* ph);

//simple fgetc wrapper to increment ph as needed
//lines is increased for every '\n' and pos is set back to zero
//else pos is incremented by 1
//returns -1 on EOF otherwise returns character
int ph_get(FILE* file, parse_helper* ph);

//removes preceding whitespace
//returns last character found or -1 if EOF
int ph_ignorews(FILE* file, parse_helper* ph) ;

//attempts to read a string from file into a buffer
//limited by maxchr and increments the parse_helper to keep track of line and current character
//assumes buffer is large enough to handle the string
//returns 0 if input is bigger than buffer
//returns -1 if read EOF and on succes or 1 on just success
int readbuf_string(FILE* file,char* buf,size_t maxchr,parse_helper* ph);

int readbuf_uint(FILE* file,size_t* pnum,size_t maxchr, parse_helper* ph);

bool is_consect_digit(char* buf);

#endif