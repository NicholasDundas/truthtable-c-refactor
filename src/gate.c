#include "gate.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

gate_return_result gate_return(gate* g) { 
    if (g != NULL) {
        size_t os; //output selector for Decoders/Multiplexers
        switch (g->kind) {
            case NOT:
                *g->params[1] = *g->params[0] ? false : true;
                break;
            case AND:
                *g->params[2] = *g->params[0] && *g->params[1];
                break;
            case OR:
                *g->params[2] = *g->params[0] || *g->params[1];
                break;
            case PASS:
                *g->params[1] = *g->params[0];
                break;
            case XOR:
                *g->params[2] = *g->params[0] ^ *g->params[1];
                break;
            case NAND:
                *g->params[2] = (*g->params[0] && *g->params[1]) ? false : true;
                break;
            case NOR:
                *g->params[2] = (*g->params[0] || *g->params[1]) ? false : true;
                break;
            case DECODER:
                for (os = g->size; os < ((size_t)1 << g->size); os++) //resetting outputs
                    *g->params[os] = 0;
                os = g->size;
                for (size_t sp = 0; sp < g->size; sp++)
                    os += (*g->params[sp] ? (size_t)1 << sp : 0);
                *g->params[os] = 1;
                break;
            case MULTIPLEXER:
                os = g->size;
                for (size_t sp = 0; sp < g->size; sp++)
                    os += (*g->params[sp] ? (size_t)1 << sp : 0);
                *g->params[g->size + ((size_t)2 << g->size)] = *g->params[os];
                break;
            default:
                return INVALID_GATE_PASSED;
        }
        return GATE_RUN_SUCCESS;
    }
        return NULL_GATE_PASSED;
}