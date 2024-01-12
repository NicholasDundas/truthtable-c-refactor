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
            return g.params[0]->value != unevaluated;
        case AND:
        case OR:
        case XOR:
        case NAND:
        case NOR:
            return g.params[0]->value != unevaluated && g.params[1]->value != unevaluated;
        case DECODER:
            for(size_t i = 0; i < g.size; i++)
                if(g.params[i]->value == unevaluated)
                    return false;
            return true;
        case MULTIPLEXER:
            for(size_t i = 0; i < g.total_size-1; i++)
                if(g.params[i]->value == unevaluated)
                    return false;
            return true;
        default:
            return false;
    }
}

const char* gate_type_to_char(kind_t type) {
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
        default:
            return "UNKNOWN";
    }
}

