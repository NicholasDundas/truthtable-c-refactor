#include "circuit.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "variable.h"
#include "gate.h"
#include "list.h"
#include "parsehelper.h"

size_t get_var(circuit* c, char* name) { 
    if (c == NULL || name == NULL) {
        return GET_VAR_NULL_PASSED;
    }
    for (size_t i = 0; i != c->var_len; ++i)
        if (strncmp(c->variables[i].letter, name, NAME_SIZE) == 0)
            return i;
    return c->var_len;
}

int insert_var(circuit* c, type_t type, char* name, bool value) {
    if (c != NULL && name != NULL) {
        if (c->var_len >= c->var_cap) {
            c->var_cap *= 2;
            c->variables = realloc(c->variables, sizeof(variable) * c->var_cap);
        }
        memcpy(&c->variables[c->var_len].letter, name, sizeof(char) * NAME_SIZE);
        c->variables[c->var_len].type = type;
        c->variables[c->var_len].value = value;
        ++c->var_len;
    }
    return 1;
}

circuit* init_circuit(void) {
    circuit* cir = malloc(sizeof(circuit));
    if (cir != NULL) {
        cir->gate_len = 0;
        cir->gate_cap = 2;
        cir->gates = malloc(sizeof(gate) * cir->gate_cap);;

        cir->var_len = 0;
        cir->var_cap = 4;
        cir->variables = malloc(sizeof(variable) * cir->var_cap);
        //Inserting Constants into Circuit
        insert_var(cir, CONST, "0", false);
        insert_var(cir, CONST, "1", true);
        insert_var(cir, DISCARD, "_", false);

        cir->input_len = 0;
        cir->output_len = 0;
    }
    return cir;
}

void free_circuit(circuit* cir) {
    if (cir != NULL) {
        free(cir->variables);
        for (size_t i = 0; i < cir->gate_len; ++i) {
            free(cir->gates[i].params); 
        }
        free(cir->gates);
        free(cir);
    }
}

int insert_gate(circuit* c, kind_t kind, variable** params, size_t size,size_t total_size) {
    if (c != NULL) {
        if (c->gate_len >= c->gate_cap) {
            c->gate_cap = (c->gate_cap << 1) - (c->gate_cap >> 1);
            c->gates = realloc(c->gates, sizeof(gate) * c->gate_cap);
        }
        c->gates[c->gate_len].kind = kind;
        c->gates[c->gate_len].params = params;
        c->gates[c->gate_len].size = size;
        c->gates[c->gate_len].total_size = total_size;
        ++c->gate_len;
        return 0;
    }
    return 1;
}

void in_circuit(circuit *cir, size_t in) {
    for (size_t k = cir->input_len - 1; k != SIZE_MAX; k--) {
        cir->variables[k + CIRCUIT_CONST_OFFSET].value = in & 1;
        in >>= 1;
    }
}

void print_circuit(FILE *file, circuit *cir)
{
    fprintf(file, "CIRCUIT\n VARIABLES:");
    for(size_t i = 0; i < cir->var_len; i++) {
        fprintf(file,"\n  ");
        print_var(file, &cir->variables[i]);
    }
    fprintf(file, "\n GATES:");
    for(size_t i = 0; i < cir->gate_len; i++) {
        fprintf(file,"\n  ");
        print_gate(file,cir->gates[i]);
    }
}

gate_return_err gate_return(gate* g) { 
    if (g == NULL) {
                return NULL_GATE_PASSED;
    }
    size_t os; //output selector for Decoders/Multiplexers    
    switch (g->kind) {
        case NOT:
            GATE_PARAM_BOOL(g,1) = GATE_PARAM_BOOL(g,0) ? false : true;
            break;
        case AND:
            GATE_PARAM_BOOL(g,2) = GATE_PARAM_BOOL(g,0) && GATE_PARAM_BOOL(g,1);
            break;
        case OR:
            GATE_PARAM_BOOL(g,2) = GATE_PARAM_BOOL(g,0) || GATE_PARAM_BOOL(g,1);
            break;
        case PASS:
            GATE_PARAM_BOOL(g,1) = GATE_PARAM_BOOL(g,0);
            break;
        case XOR:
            GATE_PARAM_BOOL(g,2) = GATE_PARAM_BOOL(g,0) ^ GATE_PARAM_BOOL(g,1);
            break;
        case NAND:
            GATE_PARAM_BOOL(g,2) = (GATE_PARAM_BOOL(g,0) && GATE_PARAM_BOOL(g,1)) ? false : true;
            break;
        case NOR:
             GATE_PARAM_BOOL(g,2) = (GATE_PARAM_BOOL(g,0) || GATE_PARAM_BOOL(g,1)) ? false : true;
            break;
        case DECODER:
            for(size_t sp = g->size; sp < g->total_size; ++sp) //reset outputs
                GATE_PARAM_BOOL(g,sp) = false;
            os = g->size;
            for (size_t sp = 0; sp < g->size; ++sp)
                os += (GATE_PARAM_BOOL(g,sp) ? 1 << sp : 0);
            GATE_PARAM_BOOL(g,os)= 1;
            break;
        case MULTIPLEXER:
        //loop until the element before the last element as the output is the true final element
            os = 0;
            for(size_t sp = 1 << g->size; sp < g->total_size; ++sp)
                os += (GATE_PARAM_BOOL(g,sp) ? 1 << (sp - (1 << g->size)) : 0);     
            GATE_PARAM_BOOL(g,g->total_size - 1) = GATE_PARAM_BOOL(g,os);
            break;
        default:
            return INVALID_GATE_PASSED;
    }
    return GATE_RUN_SUCCESS;
}

void reset_circuit(circuit *cir) {
    for(size_t i = CIRCUIT_CONST_OFFSET; i < cir->var_len; i++)
        switch(cir->variables[i].type) {
            case TEMP:
                cir->variables[i].value = unevaluated;
                break;
            default:
                continue;
        }
}


size_t out_circuit(circuit *cir) {
    //Preforming Gate Actions
    reset_circuit(cir);
    list* l = init_list();
    for (size_t c = 0; c < cir->gate_len; c++) {
        if(is_evaluable(cir->gates[c]))
            gate_return(&cir->gates[c]);
        else
            add_to_list(l,&cir->gates[c]);
        for(size_t c = l->size-1; c != SIZE_MAX ; --c) {
            gate* index = get_list(l,c);
            if(is_evaluable(*index)) {
                gate_return((gate*)remove_from_list_index(l,c));
            }
        }
    }
    size_t num = 0;
    for (size_t k = cir->input_len + cir->output_len; k != cir->input_len; k--) {
        num |= cir->variables[(k - 1) + CIRCUIT_CONST_OFFSET].value & 1;
        num <<= 1;
    }
    return num;
}

void print_gate(FILE* file,gate g) {
    fprintf(file, " GATE: ");
    fprintf(file,"%s",gate_type_to_char(g.kind));
    size_t i;
    fprintf(file, "\n      INPUT");
    if(g.kind != MULTIPLEXER) {
        for(i = 0; i < g.size; ++i) {
            fprintf(file, "\n       ");
            print_var(file, g.params[i]);
        }            
        fprintf(file, "\n      OUTPUT");
        if(g.kind != DECODER) {
            fprintf(file, "\n       ");
            print_var(file, g.params[i]);
        } else {
            for(; i < g.total_size; ++i) {
                fprintf(file, "\n       ");
                print_var(file, g.params[i]);
            }
        }
    } else {
        for(i = 0; i < ((size_t)1 << g.size); ++i) {
            fprintf(file, "\n       ");
            print_var(file, g.params[i]);
            fprintf(file, "\n");
        }
        fprintf(file, "      SELECTORS");
            for(; i < g.total_size - 1; ++i) {
                fprintf(file, "\n       ");
                print_var(file, g.params[i]);
            }
        fprintf(file, "\n      OUTPUT\n       ");
        print_var(file, g.params[i]);
    }

}

//Errors READ_VAR can throw
typedef enum { OUTPUT_EXPECTED_ERROR = 2, INPUT_EXPECTED_ERROR ,UNKNOWN_ERROR, BUFFER_OVERFLW } READ_VAR_FAIL;

//variable reading helper function
int read_var(FILE* finput, circuit* cir, char* buf,size_t i,size_t output_cap, variable** value,parse_helper* ph) { 
    int res = readbuf_string(finput, buf, NAME_SIZE,ph);
    if (res == 0)
        return BUFFER_OVERFLW;
    size_t index = get_var(cir, buf);
    //If it doesnt exist (and we need outputs), create it!
    if (index == cir->var_len) {
        insert_var(cir, TEMP, buf,false);
        *value = &cir->variables[cir->var_len - 1];
        return res != EOF ? EXIT_SUCCESS : EOF;
    }
    //Must read in an input, const, or temp as an input before the index i reaches output_cap, in which it will only accept const_out, temp, and OUTPUT
    else if (index != cir->var_len && //Make sure we found something before...
        ((i >= output_cap && output_friendly(cir->variables[index])) //Are we expecting output?
        || (i < output_cap && input_friendly(cir->variables[index])))) { //Are we expecting input?
            *value = &cir->variables[index];
            return res != EOF ? EXIT_SUCCESS : EOF;
    }
    *value = NULL;
    if (i >= output_cap && input_friendly(cir->variables[index]))
        return OUTPUT_EXPECTED_ERROR;
    if (i < output_cap && output_friendly(cir->variables[index]))
        return INPUT_EXPECTED_ERROR;
    return UNKNOWN_ERROR;
}

circuit* read_from_file(char *filename) {
    FILE* finput = fopen(filename, "r");
    if (finput == NULL) {
        fprintf(stderr, "error opening file, \"%s\": %s",filename ,strerror(errno));
        goto FAIL;
    }


    circuit* cir = init_circuit();
    parse_helper* ph = init_ph();

    size_t index = GET_VAR_NULL_PASSED;

    char* buf = malloc(NAME_SIZE + 1);

    if (!readbuf_string(finput, buf, NAME_SIZE,ph))
        goto FAIL;
    if (strncmp(buf, "INPUT", 5) == 0) {
        if (!readbuf_uint(finput, &cir->input_len, NAME_SIZE, ph))
            goto FAIL;
        if (cir->input_len > MAX_VAR_COUNT) {
            fprintf(stderr, "ERROR: input count, \"%zu\", exceeds MAX_VAR_COUNT, \"%zu\"\nLINE:%zu\nPOSITION:%zu", cir->input_len, MAX_VAR_COUNT,ph->line,ph->lastword_pos-strlen(buf));
            goto FAIL;
        }
        for (size_t i = 0; i < cir->input_len; i++) {
            if (!readbuf_string(finput, buf, NAME_SIZE,ph))
                goto FAIL;
            if ((index = get_var(cir, buf)) == cir->var_len)
                insert_var(cir, INPUT, buf, false);
            else if (index < cir->var_len) {
                fprintf(stderr, "duplicate input variable name entered : %s\nLINE:%zu\nPOSITION:%zu", buf,ph->line,ph->lastword_pos-strlen(buf));
                goto FAIL;
            }
            else {
                fprintf(stderr, "invalid input variable name entered : %s\nLINE:%zu\nPOSITION:%zu", buf,ph->line,ph->lastword_pos-strlen(buf));
                goto FAIL;
            }
        }
    }
    else {
        fprintf(stderr, "Error parsing file, \"INPUT\" indentifer not found, \"%s\" was found instead\nLINE:%zu\nPOSITION:%zu", buf,ph->line,ph->lastword_pos-strlen(buf));
        goto FAIL;
    }

    if (!readbuf_string(finput, buf, NAME_SIZE,ph))
        goto FAIL;
    if (strncmp(buf, "OUTPUT", 6) == 0) {
        if (!readbuf_uint(finput, &cir->output_len, NAME_SIZE, ph))
            goto FAIL;
        if (cir->output_len > MAX_VAR_COUNT) {
            fprintf(stderr, "ERROR: output count, \"%zu\", exceeds MAX_VAR_COUNT, \"%zu\"\nLINE:%zu\nPOSITION:%zu", cir->output_len, MAX_VAR_COUNT,ph->line,ph->lastword_pos-strlen(buf));
            goto FAIL;
        }
        for (size_t i = 0; i < cir->output_len; i++) {
            if (!readbuf_string(finput, buf, NAME_SIZE,ph))
                goto FAIL;
            if ((index = get_var(cir, buf)) == cir->var_len)
                insert_var(cir, OUTPUT, buf,false);
            else if (index < cir->var_len) {
                fprintf(stderr, "duplicate output variable name entered : %s\nLINE:%zu\nPOSITION:%zu", buf,ph->line,ph->lastword_pos-strlen(buf));
                goto FAIL;
            }
            else {
                fprintf(stderr, "invalid output variable name entered : %s\nLINE:%zu\nPOSITION:%zu", buf,ph->line,ph->lastword_pos-strlen(buf));
                goto FAIL;
            }
        }
    } else {
        fprintf(stderr,"Error parsing file, \"INPUT\" indentifer not found, \"%s\" was found instead\nLINE:%zu\nPOSITION:%zu", buf,ph->line,ph->lastword_pos-strlen(buf));
        goto FAIL;
    }


    //Read in circuit
    int read_result = 1;
    while ((read_result = readbuf_string(finput, buf, NAME_SIZE,ph)) == 1) {
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
            fprintf(stderr, "Unknown gate type : %s\nLINE:%zu\nPOSITION:%zu", buf,ph->line,ph->lastword_pos-strlen(buf));
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
                readbuf_uint(finput, &temp_gate.size, NAME_SIZE, ph);
                if (temp_gate.size > MAX_VAR_COUNT) {
                    fprintf(stderr, "gate size, \"%zu\", exceeds MAX_VAR_COUNT, \"%zu\"\nLINE:%zu\nPOSITION:%zu", temp_gate.size, MAX_VAR_COUNT,ph->line,ph->lastword_pos-strlen(buf));
                    goto FAIL;
                }
                temp_gate.total_size = temp_gate.size + ((size_t)1 << temp_gate.size) + (temp_gate.kind == MULTIPLEXER ? 1 : 0);
                output_cap = temp_gate.size + (temp_gate.kind == MULTIPLEXER ? ((size_t)1 << temp_gate.size) : 0);
                break;
        }
        temp_gate.params = malloc(sizeof(variable*) * temp_gate.total_size);
        for (size_t i = 0; i < temp_gate.total_size; ++i) {
            READ_VAR_FAIL res = read_var(finput,cir,buf,i,output_cap, &temp_gate.params[i],ph);
            if (temp_gate.params[i] == NULL) {
                switch (res) {
                    case INPUT_EXPECTED_ERROR:
                        fprintf(stderr, "could not read in variable as  \"%s\" is not of type CONST, INPUT, or TEMP\n\nLINE:%zu\nPOSITION:%zu", buf,ph->line,ph->lastword_pos-strlen(buf));
                        break;
                    case OUTPUT_EXPECTED_ERROR:
                        fprintf(stderr, "could not read in variable as  \"%s\" is not of type DISCARD, OUTPUT, or TEMP\n\nLINE:%zu\nPOSITION:%zu", buf,ph->line,ph->lastword_pos-strlen(buf));
                        break;
                    case UNKNOWN_ERROR: 
                    default:
                        break;               
                }
                goto FAIL;
            }
        }
        insert_gate(cir, temp_gate.kind, temp_gate.params, temp_gate.size, temp_gate.total_size);       
    }
    if (read_result != EOF) {
        fprintf(stderr, "error reading gates, buf = \"%s\"\nLINE:%zu\nPOSITION:%zu",buf,ph->line,ph->lastword_pos-strlen(buf));
        goto FAIL;
    }
    fclose(finput);
    free(buf); 
    return cir;
    FAIL: //Failstate exits and returns 1
    free_circuit(cir);
    cir = NULL;
    fclose(finput);
    free(buf); 
    return cir;

}
