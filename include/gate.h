#ifndef GATE_H
#define GATE_H

#include <stdbool.h>
#include <stdlib.h>


typedef enum { AND, OR, NAND, NOR, XOR, NOT, PASS, DECODER, MULTIPLEXER } kind_t; //Defines the types of gates

/* Struct gate
*
* kind_t kind is an enum that defines the type of gate we are looking at
* size is used for either the decoder or multiplexer to determine its size
* params holds pointers to integers so that mutliple Gates may point to the same variable
*
* MEMORY LAYOUT OF GATES
* - index i refers to *param[i] where that represents a variable of an appropiate type
* - Always follows inputs first followed by outputs
*
* NOT, PASS
* index 0 is input
* index 1 is output
*
* AND, OR, XOR, NAND, NOR
* index 0,1 is input
* index 2 is output
*
* DECODER
*
* MULTIPLEXER
*/
typedef struct {
    kind_t kind;
    size_t size; // indicates size of DECODER and MULTIPLEXER
    bool** params; // length determined by kind and size;
    // includes inputs and outputs, indicated by variable numbers
} gate;

//Preforms gate action on the gate pointed. Returns 0 on success and -1 if an error has occured or -3 if invalid gate
int gate_return(gate* g);

#endif