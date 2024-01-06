/*
* author: Nicholas Dundas
* date: 1/5/2024
* Truthtable program I have refactored from an older code made in my Sophmore year of College
* Takes a file as the first argument and prints out a truth table.
*
* Simple usage
* .\truthtable <FILE> 
*
*/



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "circuit.h"
#include "variable.h"


#define MAX_VAR_COUNT (size_t)32


//reads into buffer and exits if fails
int readbuf(FILE* file,void* buf,char* format,char* errmsg) { 
    int readerr = fscanf(file, format, buf);
    if (readerr <= 0) {
        perror(errmsg);
        exit(EXIT_FAILURE);
    }
    return readerr;
}


//Errors READ_VAR can throw
typedef enum { TEMP_DOESNT_EXIST, OUTPUT_EXPECTED_ERROR, INPUT_EXPECTED_ERROR ,UNKNOWN_ERROR } READ_VAR_FAIL;

//variable reading helper function
int read_var(FILE* finput, circuit* cir, char* buf,char* format,size_t i,size_t output_cap, bool** value) { 
    readbuf(finput, buf, format, "variable name read fail");
    size_t index = get_var(cir, buf);
    //If it doesnt exist (and we need outputs), create it!
    if (index == cir->var_len && i >= output_cap) {
        insert_var(cir, TEMP, buf,false);
        *value = cir->variables[cir->var_len - 1].value;
        return 0;
    }
    //Must read in an input, const, or temp as an input before the index i reaches output_cap, in which it will only accept const_out, temp, and OUTPUT
    else if (index != cir->var_len && //Make sure we found something before...
        ((i >= output_cap && output_friendly(cir->variables[index])) //Are we expecting output?
        || (i < output_cap && input_friendly(cir->variables[index])))) { //Are we expecting input?
            *value = cir->variables[index].value;
            return 0;
    }
    *value = NULL;
    if (i < output_cap)
        return TEMP_DOESNT_EXIST;
    if (i >= output_cap && input_friendly(cir->variables[index]))
        return OUTPUT_EXPECTED_ERROR;
    if (i < output_cap && output_friendly(cir->variables[index]))
        return INPUT_EXPECTED_ERROR;
    return UNKNOWN_ERROR;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage : %s <FILE_NAME>\n", argv[0]);
        goto FAIL;
    }

    FILE* finput = fopen(argv[1], "r");
    if (finput == NULL) {
        fprintf(stderr, "error opening file, \"%s\": %s",argv[1] ,strerror(errno));
    }

    //contains "%(NAME_SIZE)s" where (NAME_SIZE) is a macro defined in variable.h inserted via snprintf
    char nameformat[16];
    snprintf(nameformat, sizeof(nameformat), "%%%zus", NAME_SIZE);

    circuit* cir = init_circuit();

    size_t input_len = 0, output_len = 0, index = GET_VAR_NULL_PASSED;

    char* buf = malloc((NAME_SIZE + 1) * sizeof(char));

    readbuf(finput, buf, nameformat, "error reading \"INPUT\" from file");
    if (strncmp(buf, "INPUT", 5) == 0) {
        readbuf(finput, &input_len, "%zu", "error reading variable N count from file");
        if (input_len > MAX_VAR_COUNT) {
            fprintf(stderr, "count, \"%zu\", exceeds MAX_VAR_COUNT, \"%zu\"", input_len, MAX_VAR_COUNT);
            goto FAIL;
        }
        for (size_t i = 0; i < input_len; i++) {
            readbuf(finput, buf, nameformat, "input value name read fail");
            if ((index = get_var(cir, buf)) == cir->var_len)
                insert_var(cir, INPUT, buf, false);
            else if (index < cir->var_len) {
                fprintf(stderr, "duplicate input variable name entered : %s", buf);
                goto FAIL;
            }
            else {
                fprintf(stderr, "invalid input variable name entered : %s", buf);
                goto FAIL;
            }
        }
    }
    else {
        fprintf(stderr, "Error parsing file, \"INPUT\" indentifer not found, \"%s\" was found instead", buf);
        goto FAIL;
    }

    readbuf(finput, buf, nameformat, "error reading \"OUTPUT\" from file");
    if (strncmp(buf, "OUTPUT", 6) == 0) {
        readbuf(finput, &output_len, "%zu", "error reading variable N count from file");
        if (output_len > MAX_VAR_COUNT) {
            fprintf(stderr, "count, \"%zu\", exceeds MAX_VAR_COUNT, \"%zu\"", output_len, MAX_VAR_COUNT);
            goto FAIL;
        }
        for (size_t i = 0; i < output_len; i++) {
            readbuf(finput, buf, nameformat, "output value name read fail");
            if ((index = get_var(cir, buf)) == cir->var_len)
                insert_var(cir, OUTPUT, buf,false);
            else if (index < cir->var_len) {
                fprintf(stderr, "duplicate output variable name entered : %s", buf);
                goto FAIL;
            }
            else {
                fprintf(stderr, "invalid output variable name entered : %s", buf);
                goto FAIL;
            }
        }
    } else {
        fprintf(stderr,"Error parsing file, \"INPUT\" indentifer not found, \"%s\" was found instead", buf);
        goto FAIL;
    }


    //Read in circuit
    int read_result = EOF;
    while ((read_result = fscanf(finput, nameformat, buf)) == 1) {
        gate temp_gate = {.kind = 0, .params = NULL, .size = 0};
        if (strncmp(buf, "NOT", 3) == 0) temp_gate.kind = NOT;
        else if (strncmp(buf, "PASS", 4) == 0) temp_gate.kind = PASS;
        else if (strncmp(buf, "AND", 3) == 0) temp_gate.kind = AND;
        else if (strncmp(buf, "OR", 2) == 0) temp_gate.kind = OR;
        else if (strncmp(buf, "NAND", 4) == 0) temp_gate.kind = NAND;
        else if (strncmp(buf, "NOR", 3) == 0) temp_gate.kind = NOR;
        else if (strncmp(buf, "XOR", 3) == 0) temp_gate.kind = XOR;
        else if (strncmp(buf, "DECODER",7) == 0) temp_gate.kind = DECODER;
        else if (strncmp(buf, "MULTIPLEXER",11) == 0) temp_gate.kind = MULTIPLEXER;
        else {
            fprintf(stderr, "Unknown gate type : %s", buf);
            goto FAIL;
        }
        size_t size = 2, total_size; //Size of params (includes all inputs, outputs, selectors, etc)
        size_t output_cap = 1; //Index of where outputs may start
        switch (temp_gate.kind) {
            case AND:
            case OR:
            case NAND:
            case NOR:
            case XOR:
                size = 3; //2 inputs [0,1,...], 1 output.... [...,2]
                output_cap = 2;
                __attribute__ ((fallthrough));
            case NOT:
            case PASS:
                total_size = size;
                break;
            case DECODER:
            case MULTIPLEXER:
                readbuf(finput, &size, "%zu", "error reading gate variable N count");
                if (size > MAX_VAR_COUNT) {
                    fprintf(stderr, "gate size, \"%zu\", exceeds MAX_VAR_COUNT, \"%zu\"", size, MAX_VAR_COUNT);
                    goto FAIL;
                }
                total_size = size + ((size_t)1 << size) + (temp_gate.kind == MULTIPLEXER ? 1 : 0);
                output_cap = size + (temp_gate.kind == MULTIPLEXER ? ((size_t)1 << size) : 0);
                break;
        }
        temp_gate.params = malloc(sizeof(bool*) * total_size);
        for (size_t i = 0; i < size; ++i) {
            READ_VAR_FAIL res = read_var(finput, cir, buf, nameformat, i, output_cap,&temp_gate.params[i]);
            if (temp_gate.params[i] == NULL) {
                if (res == TEMP_DOESNT_EXIST) {
                    fprintf(stderr, "could not read in variable as \"%s\" has not been declared as an output before\n", buf);
                    goto FAIL;
                } else if (res == INPUT_EXPECTED_ERROR) {
                    fprintf(stderr, "could not read in variable as  \"%s\" is not of type CONST, INPUT, or TEMP\n", buf);
                    goto FAIL;
                } else if (res == OUTPUT_EXPECTED_ERROR) {
                    fprintf(stderr, "could not read in variable as  \"%s\" is not of type DISCARD, OUTPUT, or TEMP\n", buf);
                    goto FAIL;
                } else if (res == UNKNOWN_ERROR) {
                    fprintf(stderr, "could not read in variable : %s\n", buf);
                    goto FAIL;
                }
            }
        }
        insert_gate(cir, temp_gate.kind, temp_gate.params, size);       
    }
    if (read_result != EOF) {
        fprintf(stderr, "error reading gates, buf = \"%s\"\n%s",buf,strerror(errno));
        goto FAIL;
    }
    fclose(finput);

    const size_t constant_offset = 3; //Offsets us from the first 3 variables
    for (size_t i = 0; i < (size_t)1 << input_len; i++) {
        //Setting Inputs to an increasing Number i
        size_t num = i;
        for (size_t k = 0; k < input_len; k++) {
            *cir->variables[k + constant_offset].value = num & 1;
            num >>= 1;
        }

        //Preforming Gate Actions
        for (size_t c = 0; c < cir->gate_len; c++) {
            if (gate_return(&(cir->gates[c])) != 0) {
                fprintf(stderr, "Invalid gate attempted run, %d", cir->gates[c].kind);
                goto FAIL;
            }
        }

        //Printing Current State
        for (size_t k = 0; k < input_len; ++k) {
            printf("%d ", *cir->variables[k + constant_offset].value);
        }
        printf("|");
        for (size_t k = input_len; k < input_len + output_len; k++) {
            printf(" %d", *cir->variables[k + constant_offset].value);
        }
        printf("\n");
    }
    free_circuit(cir);
    return EXIT_SUCCESS;

FAIL: //Failstate
    return EXIT_FAILURE;
}