#include "variable.h"


bool output_friendly(variable v) {
    return (v.type == OUTPUT || v.type == CONST_OUT || v.type == TEMP);
}

bool input_friendly(variable v) {
    return (v.type == INPUT || v.type == CONST || v.type == TEMP);
}