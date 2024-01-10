#include "circuit.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "variable.h"
#include "gate.h"


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
        memcpy(&c->variables[c->var_len], name, sizeof(char) * NAME_SIZE);
        c->variables[c->var_len].type = type;
        c->variables[c->var_len].value = malloc(sizeof(bool));
        *c->variables[c->var_len].value = value;
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
        insert_var(cir, CONST_OUT, "_", false);
    }
    return cir;
}

void free_circuit(circuit* cir) {
    if (cir != NULL) {
        for (size_t i = 0; i < cir->var_len; ++i) {
            free(cir->variables[i].value); 
        }
        free(cir->variables);
        for (size_t i = 0; i < cir->gate_len; ++i) {
            free(cir->gates[i].params); 
        }
        free(cir->gates);
        free(cir);
    }
}

int insert_gate(circuit* c, kind_t kind, size_t* params, size_t size,size_t total_size) {
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

void print_circuit(FILE* file, circuit* cir) {
    fprintf(file, "CIRCUIT\n VARIABLES:");
    for(size_t i = 0; i < cir->var_len; i++) {
        fprintf(file,"\n  ");
        print_var(file, &cir->variables[i]);
    }
    fprintf(file, "\n GATES:");
    for(size_t i = 0; i < cir->gate_len; i++) {
        fprintf(file,"\n  ");
        print_gate(file,cir,&cir->gates[i]);
    }
}

#define GATE_PARAM_BOOL(CIRCUIT_PTR,GATE_PTR,INDEX) *(CIRCUIT_PTR)->variables[(GATE_PTR)->params[(INDEX)]].value

gate_return_err gate_return(circuit* cir, gate* g) { 
    if (g == NULL) {
                return NULL_GATE_PASSED;
    }
    size_t os; //output selector for Decoders/Multiplexers    
    switch (g->kind) {
        case NOT:
            GATE_PARAM_BOOL(cir,g,1) = GATE_PARAM_BOOL(cir,g,0) ? false : true;
            break;
        case AND:
            GATE_PARAM_BOOL(cir,g,2) = GATE_PARAM_BOOL(cir,g,0) && GATE_PARAM_BOOL(cir,g,1);
            break;
        case OR:
            GATE_PARAM_BOOL(cir,g,2) = GATE_PARAM_BOOL(cir,g,0) || GATE_PARAM_BOOL(cir,g,1);
            break;
        case PASS:
            GATE_PARAM_BOOL(cir,g,1) = GATE_PARAM_BOOL(cir,g,0);
            break;
        case XOR:
            GATE_PARAM_BOOL(cir,g,2) = GATE_PARAM_BOOL(cir,g,0) ^ GATE_PARAM_BOOL(cir,g,1);
            break;
        case NAND:
            GATE_PARAM_BOOL(cir,g,2) = (GATE_PARAM_BOOL(cir,g,0) && GATE_PARAM_BOOL(cir,g,1)) ? false : true;
            break;
        case NOR:
             GATE_PARAM_BOOL(cir,g,2) = (GATE_PARAM_BOOL(cir,g,0) || GATE_PARAM_BOOL(cir,g,1)) ? false : true;
            break;
        case DECODER:
            for(size_t sp = g->size; sp < g->total_size; ++sp) //reset outputs
                GATE_PARAM_BOOL(cir,g,sp) = false;
            os = g->size;
            for (size_t sp = 0; sp < g->size; ++sp)
                os += (GATE_PARAM_BOOL(cir,g,sp) ? 1 << sp : 0);
            GATE_PARAM_BOOL(cir,g,os)= 1;
            break;
        case MULTIPLEXER:
        //loop until the element before the last element as the output is the true final element
            os = 0;
            for(size_t sp = 1 << g->size; sp < g->total_size; ++sp)
                os += (GATE_PARAM_BOOL(cir,g,sp) ? 1 << (sp - (1 << g->size)) : 0);     
            GATE_PARAM_BOOL(cir,g,g->total_size - 1) = GATE_PARAM_BOOL(cir,g,os);
            break;
        default:
            return INVALID_GATE_PASSED;
    }
    return GATE_RUN_SUCCESS;
}

void print_gate(FILE* file,circuit* cir,gate* g) {
    fprintf(file, " GATE: ");
    fprintf(file,"%s",gate_type_to_char(g->kind));
    size_t i;
    fprintf(file, "\n      INPUT");
    if(g->kind != MULTIPLEXER) {
        for(i = 0; i < g->size; ++i) {
            fprintf(file, "\n       ");
            print_var(file, &cir->variables[g->params[i]]);
        }            
        fprintf(file, "\n      OUTPUT");
        if(g->kind != DECODER) {
            print_var(file, &cir->variables[g->params[i]]);
        } else {
            for(; i < g->total_size; ++i) {
                fprintf(file, "\n       ");
                print_var(file, &cir->variables[g->params[i]]);
            }
        }
    } else {
        for(i = 0; i < ((size_t)1 << g->size); ++i) {
            fprintf(file, "       ");
            print_var(file, &cir->variables[g->params[i]]);
            fprintf(file, "\n");
        }
        fprintf(file, "      SELECTORS");
            for(; i < g->total_size - 1; ++i) {
                fprintf(file, "\n       ");
                print_var(file, &cir->variables[g->params[i]]);
            }
        fprintf(file, "\n      OUTPUT\n       ");
        print_var(file, &cir->variables[g->params[i]]);
    }

}