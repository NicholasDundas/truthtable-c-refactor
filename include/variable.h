#ifndef VARIABLE_H
#define VARIABLE_H

#include <stdlib.h>
#include <stdio.h>

#include "maybe.h"

//Maximum length of name of a variable including null terminator
#define NAME_SIZE (size_t)64 

//Defines the types of variables
typedef enum { INPUT, TEMP, OUTPUT, CONST, DISCARD } type_t; 
typedef enum { var_false = 0, var_true = 1, var_uneval} var_result;
// Holds information about name, type, and a boolean in the heap
typedef struct {
    char letter[NAME_SIZE + 1]; //name of variable
    var_result value; //holds value
    type_t type; //type of variable
} variable;

//returns whether a value is OUTPUT, DISCARD, or TEMP
bool output_friendly(variable v);

//returns whether a value is INPUT, CONST, or TEMP
bool input_friendly(variable v);

//Debug print
//Example:
//[NAME: John, true (0xA421F412), INPUT]
void print_var(FILE* file,variable* v);

//Debug print
//Example:
//[true (0xA421F412)]
void print_bool(FILE* file,bool* v);

//converts type_t to a string
const char* variable_type_to_char(type_t type);
#endif
