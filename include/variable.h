#ifndef VARIABLE_H
#define VARIABLE_H

#include <stdbool.h>
#include <stdlib.h>

//Maximum length of name of a variable including null terminator
#define NAME_SIZE (size_t)64 

//Defines the types of variables
typedef enum { INPUT, TEMP, OUTPUT, CONST, CONST_OUT } type_t; 

// Holds information about name, type, and a boolean in the heap
typedef struct {
    char letter[NAME_SIZE]; //name of variable
    bool* value; //holds value
    type_t type; //type of variable
} variable;

//returns whether a value is OUTPUT, CONST_OUT, or TEMP
bool output_friendly(variable v);

//returns whether a value is INPUT, CONST, or TEMP
bool input_friendly(variable v);

#endif