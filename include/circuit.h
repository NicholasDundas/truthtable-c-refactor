#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <stdbool.h>
#include <stdlib.h>

#include "variable.h"
#include "gate.h"

/*
* Circuit struct to hold information about gates and variables
*/
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
    size_t var_len; //Variable length
    size_t var_cap; //Capacity of variables* (used for realloc efficiency)
} circuit;

#define INSERT_GATE_FAILURE 1
//attempts to insert a variable into c
//maximum length of name is defined by NAME_SIZE in variable.h
//returns 0 on success or INSERT_GATE_FAILURE on failure
int insert_var(circuit* c, type_t type, char* name, bool value); 

//Initialize an Empty Circuit
circuit* init_circuit(void); 

//Free a circuit (Must be initialized)
void free_circuit(circuit* cir);

//Error Result of GET_VAR
#define GET_VAR_NULL_PASSED SIZE_MAX

//Returns var_len of circuit on failure or GET_VAR_NULL_PASSED if a null was passed to either parameter
size_t get_var(circuit* c, char* name); 


#define INSERT_VAR_FAILURE 1
//attempts to add gate type of kind to c and returns 0 on success or 1 on failure
//size and params must match with gate type or UB will occur
int insert_gate(circuit* c, kind_t kind, bool** params, size_t size); 
#endif