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
        return 0;
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

int insert_gate(circuit* c, kind_t kind, bool** params, size_t size) {
    if (c != NULL) {
        if (c->gate_len >= c->gate_cap) {
            c->gate_cap *= 2;
            c->gates = realloc(c->gates, sizeof(gate) * c->gate_cap);
        }
        c->gates[c->gate_len].kind = kind;
        c->gates[c->gate_len].params = params;
        c->gates[c->gate_len].size = size;
        ++c->gate_len;
        return 0;
    }
    return 1;
}