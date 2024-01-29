#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <stdlib.h>
#include <stdint.h>

#include "list.h"
#include "hashtable.h"
#include "gate.h"
#include "parsehelper.h"

#define MAX_VAR_COUNT (size_t)32

//Defines the array index where input variables begin
#define CIRCUIT_CONST_OFFSET (size_t)3

//whether a circuit is initialized, declared, or defined
typedef enum { cir_init,cir_declared, cir_defined, cir_freed} cir_type; 

//Circuit struct to hold information about gates and variables
typedef struct Circuit {
    gate* gates; //All Gates
    size_t gate_len; //Amount of circuits to run
    size_t gate_cap; //Capacity of gate* (used for realloc efficiency)

    /*
    - First Three are Constants (1, 0, Discard)
    - Inputs declared on the first line
    - Then Outputs declared on the next
    - Then Temporary Variables declared as needed
    */
    variable** variables;
    size_t var_cap; //Capacity of variables* (used for realloc efficiency)
    size_t var_len; //Variable length

    //amount of input variables
    //amount of output variables
    size_t input_len;
    size_t output_len;
    char* name;
    hashtable circuit_dictionary; //contains circuits
    hashtable* inherited_dictionary; //contains circuits we have inherited from the outerscope

    cir_type type;
    size_t line_declared;
    size_t pos_declared;

    size_t line_defined;
    size_t pos_defined;
} circuit;


#define CIRCUIT_VAR_BOOL(CIR_PTR,INDEX) (CIR_PTR)->variables[(INDEX)]->value

//Attempts to insert a variable into c
//Returns 1 on success or 0 on failure
int insert_var(circuit* c, var_type type, char* name, var_result value); 

//Initialize an Empty Circuit
//returns 0 on success or 1 on failure
int init_circuit(circuit* cir,const char* name,hashtable* newdict);

//Free a circuit (Must be initialized)
void free_circuit(circuit* cir);

//Error Result of GET_VAR
#define GET_VAR_NULL_PASSED SIZE_MAX

//Returns var_len of circuit on failure or GET_VAR_NULL_PASSED if a null was passed to either parameter
size_t get_var(circuit* c, char* name); 

//converts gate_type to char
const char* gate_type_to_char(gate_type type);

#define INSERT_VAR_FAILURE 1
//attempts to add gate type of kind to c and returns 0 on success or 1 on failure
//size and params must match with gate type or UB will occur
int insert_gate(circuit* c, gate_type kind, variable** params, size_t size, size_t total_size,circuit* cir); 


//reset all temp variables to initial states
void reset_circuit(circuit* cir);

//Output of a circuit given as a number in the range of size_t
size_t out_circuit(circuit* cir);

//Input a circuit, any bits bigger than 2^(input_len) are truncated and ignored
void in_circuit(circuit* cir,size_t in);

//returns whether a circuit with an equivalent name exists in the circuit dictionary
bool declared_in_scope(circuit cir, char* name);

//returns whether a circuit with an equivalent name exists in the circuit dictionary or the inherited one
bool declared(circuit cir,char* name);

//returns 
circuit* get_circuit_reference(circuit* c,char* name);

//debug print
//Prints all variables followed by gates
void print_circuit(FILE* file,circuit* cir);


//read in a circuit from a file
circuit* read_from_file_name(char *filename); 

bool valid_circuit(circuit c) {
    
}

//parses a statement
//0 is failure
//1 is success
//2 is end of a circuit function statement
int parse_statement(FILE* finput, char** buf,size_t* bufsize,parse_helper*ph,circuit* c);
#endif
