#include "gate.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "circuit.h"



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

