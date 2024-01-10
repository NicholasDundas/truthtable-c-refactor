#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <stdbool.h>
#include <stdlib.h>

#include "variable.h"
#include "gate.h"

#define MAX_VAR_COUNT (size_t)32

//Defines the array index where input variables begin
#define CIRCUIT_CONST_OFFSET (size_t)3

//Circuit struct to hold information about gates and variables
typedef struct {
    gate* gates; //All Gates
    size_t gate_len; //Amount of circuits to run
    size_t gate_cap; //Capacity of gate* (used for realloc efficiency)

    /*
    - First Three are Constants (1, 0, Discard)
    - Inputs declared on the first line
    - Then Outputs declared on the next
    - Then Temporary Variables declared as needed
    */
    variable* variables;
    size_t var_cap; //Capacity of variables* (used for realloc efficiency)
    size_t var_len; //Variable length
} circuit;


#define INSERT_GATE_FAILURE 1
//Attempts to insert a variable into c
//Maximum length of name is defined by NAME_SIZE in variable.h
//Returns 0 on success or INSERT_GATE_FAILURE on failure
int insert_var(circuit* c, type_t type, char* name, bool value); 

//Initialize an Empty Circuit
circuit* init_circuit(void); 

//Free a circuit (Must be initialized)
void free_circuit(circuit* cir);

//Error Result of GET_VAR
#define GET_VAR_NULL_PASSED SIZE_MAX

//Returns var_len of circuit on failure or GET_VAR_NULL_PASSED if a null was passed to either parameter
size_t get_var(circuit* c, char* name); 

//converts kind_t to char
const char* gate_type_to_char(kind_t type);

#define INSERT_VAR_FAILURE 1
//attempts to add gate type of kind to c and returns 0 on success or 1 on failure
//size and params must match with gate type or UB will occur
int insert_gate(circuit* c, kind_t kind, size_t* params, size_t size, size_t total_size); 

typedef enum { GATE_RUN_SUCCESS, INVALID_GATE_PASSED, NULL_GATE_PASSED  } gate_return_err;

//Preforms gate action on the gate pointed. returns the result 
gate_return_err gate_return(circuit* cir,gate* g);

//debug print
void print_circuit(FILE* file,circuit* cir);

//Debug printing of a gate
//Example:
//GATE: AND
//  INPUT
//  - [NAME: Variable1, true (0xA421F412), INPUT]
//  - [NAME: Variable2, false (0xA421F412), INPUT]
//  OUTPUT
//  - [NAME: Variable3, false (0xA421F412), OUTPUT]
void print_gate(FILE* file,circuit* cir ,gate* g);
#endif