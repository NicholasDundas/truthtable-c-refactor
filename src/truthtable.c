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
#include <ctype.h>

#include "circuit.h"
#include "variable.h"


//reads into buffer and exits if fails
/*int readbuf(FILE* file,void* buf,char* format,char* errmsg) { 
    int readerr = fscanf(file, format, buf);
    if (readerr <= 0) {
        perror(errmsg);
        exit(EXIT_FAILURE);
    }
    return readerr;
}*/

typedef struct {
    size_t line;
    size_t pos;
} parse_helper;

parse_helper* init_ph() {
    parse_helper* tmp = malloc(sizeof(parse_helper));
    tmp->line = 1;
    tmp->pos = 1;
    return tmp;
}

//attempts to read a buffer into a string
//limited by maxchr and increments the parse_helper to keep track of line and current character
int readbuf_string(FILE* file,char* buf,size_t maxchr,const char* errmsg, parse_helper* ph) { 
    int c;
    while((c = fgetc(file)) != EOF && isspace(c)) { //remove preceding whitespace
        if(c != '\n')
            ++ph->pos;
        else {
            ++ph->line;
            ph->pos = 1;
        }
    }
    size_t count = 0;
    if(c != EOF) {
        buf[0] =  c;
        count = 1;
    }
    while(count < maxchr && (c = fgetc(file)) != EOF && isalnum(c)) {
        ++ph->pos;
        buf[count++] = c;
    }
    buf[count] = '\0';
    if(c == '\n') {
            ++ph->line;
            ph->pos = 1;
    }        
    if (count >= maxchr) {
        fprintf(stderr,"ERROR: %s\nBUFFER:%s\nLINE:%zu\nPOSITION:%zu\n",errmsg,buf,ph->line,ph->pos);
        exit(EXIT_FAILURE);
    }
    if(c == EOF)
        return -1;
    return 1;
}

int readbuf_uint(FILE* file,size_t* pnum,size_t maxchr,const char* errmsg, parse_helper* ph) { 
    int c;
    int num = 0;
    while((c = fgetc(file)) != EOF && isspace(c)) { //remove preceding whitespace
        if(c != '\n')
            ++ph->pos;
            else {
                ++ph->line;
                ph->pos = 1;
            }
    }
    size_t count = 0;
    bool flip = false;
    if(c != EOF && c == '-')
        flip = true;
    else if (c != EOF) {
        ++ph->pos;
        num = (num * 10) + c - '0';
    }
    while(count < maxchr && (c = fgetc(file)) != EOF && isdigit(c)) {
             flip = true;
        num = (num * 10) + c - '0';
        ++ph->pos;
        ++count;
    }
    if(c == '\n') {
        ++ph->line;
        ph->pos = 1;
    }    
    if (c != EOF && (flip || !isspace(c))) {
        fprintf(stderr,"ERROR: %s\nNUM:%d\nLINE:%zu\nPOSITION:%zu\n",errmsg,num,ph->line,ph->pos);
        exit(EXIT_FAILURE);
    }
    *pnum = num;
    if(c == EOF)
        return -1;
    return 1;
}

//Errors READ_VAR can throw
typedef enum { TEMP_DOESNT_EXIST = 1, OUTPUT_EXPECTED_ERROR, INPUT_EXPECTED_ERROR ,UNKNOWN_ERROR } READ_VAR_FAIL;

//variable reading helper function
int read_var(FILE* finput, circuit* cir, char* buf,size_t i,size_t output_cap, size_t* value,parse_helper* ph) { 
    readbuf_string(finput, buf, NAME_SIZE, "variable name read fail",ph);
    size_t index = get_var(cir, buf);
    //If it doesnt exist (and we need outputs), create it!
    if (index == cir->var_len && i >= output_cap) {
        insert_var(cir, TEMP, buf,false);
        *value = cir->var_len - 1;
        return 0;
    }
    //Must read in an input, const, or temp as an input before the index i reaches output_cap, in which it will only accept const_out, temp, and OUTPUT
    else if (index != cir->var_len && //Make sure we found something before...
        ((i >= output_cap && output_friendly(cir->variables[index])) //Are we expecting output?
        || (i < output_cap && input_friendly(cir->variables[index])))) { //Are we expecting input?
            *value = index;
            return 0;
    }
    *value = SIZE_MAX;
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
        goto FAIL;
    }


    circuit* cir = init_circuit();
    parse_helper* ph = init_ph();

    size_t input_len = 0, output_len = 0, index = GET_VAR_NULL_PASSED;

    char* buf = malloc((NAME_SIZE + 1) * sizeof(char));

    readbuf_string(finput, buf, NAME_SIZE, "error reading \"INPUT\" from file",ph);
    if (strncmp(buf, "INPUT", 5) == 0) {
        readbuf_uint(finput, &input_len, NAME_SIZE, "could not read amount of input variables",ph);
        if (input_len > MAX_VAR_COUNT) {
            fprintf(stderr, "ERROR: count, \"%zu\", exceeds MAX_VAR_COUNT, \"%zu\"\nLINE:%zu\nPOSITION:%zu", input_len, MAX_VAR_COUNT,ph->line,ph->pos);
            goto FAIL;
        }
        for (size_t i = 0; i < input_len; i++) {
            readbuf_string(finput, buf, NAME_SIZE, "input value name read fail",ph);
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

    readbuf_string(finput, buf, NAME_SIZE, "error reading \"OUTPUT\" from file",ph);
    if (strncmp(buf, "OUTPUT", 6) == 0) {
        readbuf_uint(finput, &output_len, NAME_SIZE, "could not read amount of output variables",ph);
        if (output_len > MAX_VAR_COUNT) {
            fprintf(stderr, "count, \"%zu\", exceeds MAX_VAR_COUNT, \"%zu\"", output_len, MAX_VAR_COUNT);
            goto FAIL;
        }
        for (size_t i = 0; i < output_len; i++) {
            readbuf_string(finput, buf, NAME_SIZE, "output value name read fail",ph);
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
    int read_result = 1;
    while ((read_result = readbuf_string(finput, buf, NAME_SIZE,"gate identifier error",ph)) == 1) {
        gate temp_gate = {.kind = 0, .params = NULL, .size = 0, .total_size = 0};
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
        temp_gate.size = 1; temp_gate.total_size = 2; //Size of params (includes all inputs, outputs, selectors, etc)
        size_t output_cap = 1; //Index of where outputs may start
        switch (temp_gate.kind) {
            case AND:
            case OR:
            case NAND:
            case NOR:
            case XOR:
                temp_gate.size = 2;
                temp_gate.total_size = 3; //2 inputs params[0,1,...], 1 output.... params[...,2]
                output_cap = 2;
                __attribute__ ((fallthrough));
            case NOT:
            case PASS:
                break;
            case DECODER:
            case MULTIPLEXER:
                readbuf_uint(finput, &temp_gate.size, NAME_SIZE, "error reading gate variable N count",ph);
                if (temp_gate.size > MAX_VAR_COUNT) {
                    fprintf(stderr, "gate size, \"%zu\", exceeds MAX_VAR_COUNT, \"%zu\"", temp_gate.size, MAX_VAR_COUNT);
                    goto FAIL;
                }
                temp_gate.total_size = temp_gate.size + ((size_t)1 << temp_gate.size) + (temp_gate.kind == MULTIPLEXER ? 1 : 0);
                output_cap = temp_gate.size + (temp_gate.kind == MULTIPLEXER ? ((size_t)1 << temp_gate.size) : 0);
                break;
        }
        temp_gate.params = malloc(sizeof(size_t*) * temp_gate.total_size);
        for (size_t i = 0; i < temp_gate.total_size; ++i) {
            READ_VAR_FAIL res = read_var(finput,cir,buf,i,output_cap, &temp_gate.params[i],ph);
            if (temp_gate.params[i] == SIZE_MAX) {
                switch (res) {
                    case TEMP_DOESNT_EXIST:
                        fprintf(stderr, "could not read in variable as \"%s\" has not been declared as an output before\n", buf);
                        break;
                    case INPUT_EXPECTED_ERROR:
                        fprintf(stderr, "could not read in variable as  \"%s\" is not of type CONST, INPUT, or TEMP\n", buf);
                        break;
                    case OUTPUT_EXPECTED_ERROR:
                        fprintf(stderr, "could not read in variable as  \"%s\" is not of type DISCARD, OUTPUT, or TEMP\n", buf);
                        break;
                    case UNKNOWN_ERROR: 
                    default:
                        fprintf(stderr, "could not read in variable : %s\n", buf);
                        break;               
                }
                goto FAIL;
            }
        }
        insert_gate(cir, temp_gate.kind, temp_gate.params, temp_gate.size, temp_gate.total_size);       
    }
    if (read_result != EOF) {
        fprintf(stderr, "error reading gates, buf = \"%s\"\n%s",buf,strerror(errno));
        goto FAIL;
    }
    fclose(finput);
    FILE* output = stdout;
    for (size_t i = 0; i < (size_t)1 << input_len; i++) {
        //Setting Inputs to an increasing Number i
        size_t num = i;
        for (size_t k = input_len - 1; k != SIZE_MAX; k--) {
            *cir->variables[k + CIRCUIT_CONST_OFFSET].value = num & 1;
            num >>= 1;
        }

        //Preforming Gate Actions
        for (size_t c = 0; c < cir->gate_len; c++) {
            if (gate_return(cir,&(cir->gates[c])) != 0) {
                fprintf(stderr, "Invalid gate attempted run, %d", cir->gates[c].kind);
                goto FAIL;
            }
        }

        //Printing Current State
        if(output == NULL) {
            goto FAIL;
        } 
        for (size_t k = 0; k < input_len; ++k) {
            fprintf(output,"%d ", *cir->variables[k + CIRCUIT_CONST_OFFSET].value);
        }
        fprintf(output,"|");
        for (size_t k = input_len; k < input_len + output_len; k++) {
            fprintf(output," %d", *cir->variables[k + CIRCUIT_CONST_OFFSET].value);
        }
        fprintf(output,"\n");
    }
    fprintf(output,"\n\n");
    print_circuit(output,cir);
    if(output != stderr && output != stdout)
        fclose(output);
    free_circuit(cir);
    return EXIT_SUCCESS;

FAIL: //Failstate exits and returns 1
    free_circuit(cir);
    return EXIT_FAILURE;

}