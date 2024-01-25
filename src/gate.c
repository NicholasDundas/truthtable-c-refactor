#include "gate.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "circuit.h"

bool is_evaluable(gate g) {
    switch (g.kind) {
        case NOT:
        case PASS:
            return g.params[0]->value != var_uneval;
        case AND:
        case OR:
        case XOR:
        case NAND:
        case NOR:
            return g.params[0]->value != var_uneval && g.params[1]->value != var_uneval;
        case DECODER:
            for(size_t i = 0; i < g.size; ++i) {
                if(g.params[i]->value == var_uneval) {
                    return false;
                }
            }
            return true;
        case MULTIPLEXER:
            for(size_t i = 0; i < g.total_size-1; ++i) {
                if(g.params[i]->value == var_uneval) {
                    return false;
                }
            }
            return true;
        case CIR_PTR:
            for(size_t i = 0; i < g.cir_ptr->input_len; ++i)
                if(g.params[i]->value == var_uneval) {
                    return false;
                }
            return true;
        default:
            return false;
    }
}

const char* gate_type_to_char(gate_type type) {
   switch (type) {
        case NOT:
            return "NOT";
        case AND:
            return "AND";
        case OR:
            return "OR";
        case PASS:
            return "PASS";
        case XOR:
            return "XOR";
        case NAND:
            return "NAND";
        case NOR:
            return "NOR";
        case DECODER:
            return "DECODER";
        case MULTIPLEXER:
            return "MULTIPLEXER";
        case CIR_PTR:
            return "CIRCUIT";
        default:
            return "UNKNOWN";
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
            for(size_t sp = 1 << g->size; sp < g->total_size-1; ++sp) {
                os += (GATE_PARAM_BOOL(g,sp) ? (1 << (sp - (1 << g->size))) : 0);   
            }  
            GATE_PARAM_BOOL(g,g->total_size - 1) = GATE_PARAM_BOOL(g,os);
            break;
        case CIR_PTR:
            for(size_t i = 0; i < g->cir_ptr->input_len; ++i)
                g->cir_ptr->variables[i + CIRCUIT_CONST_OFFSET]->value = GATE_PARAM_BOOL(g,i);
            reset_circuit(g->cir_ptr);
            out_circuit(g->cir_ptr);
            for(size_t i = g->cir_ptr->var_len-CIRCUIT_CONST_OFFSET; i > g->cir_ptr->input_len; --i)
                g->cir_ptr->variables[i + CIRCUIT_CONST_OFFSET]->value = GATE_PARAM_BOOL(g,i);
            break;
        default:
            return INVALID_GATE_PASSED;
    }
    return GATE_RUN_SUCCESS;
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
