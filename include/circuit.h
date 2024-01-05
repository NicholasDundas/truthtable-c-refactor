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

//returns 0 on success, -1 on null circuit or name passed, or -2 on memory allocation failure
int insert_var(circuit* c, type_t type, char* name, bool value); 

//Initialize an Empty Circuit
circuit* init_circuit(void); 

//Free a circuit (Must be initialized)
void free_circuit(circuit* cir);

//Returns var_len of circuit on failure or GET_VAR_NULL_PASSED if a null was passed to either parameter
size_t get_var(circuit* c, char* name); 

//Error Result of GET_VAR
#define GET_VAR_NULL_PASSED SIZE_MAX

//returns -1 on null gate passed or -2 on memory allocation failure
int insert_gate(circuit* c, kind_t kind, bool** params, size_t size); 
#endif