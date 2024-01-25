#include "func.h"

#include <string.h>

bool int_eq(void *a, void *b) {
    return *(int*)a == *(int*)b;
}

bool bool_eq(void *a, void *b) {
    return *(bool*)a == *(bool*)b;
}

bool size_t_eq(void *a, void *b) {
    return *(size_t*)a == *(size_t*)b;
}

bool char_eq(void *a, void *b) {
    return *(char*)a == *(char*)b;
}

bool str_eq(void *a, void *b) {
    return strcmp((char*)a,(char*)b) == 0;
}

bool long_eq(void *a, void *b) {
    return *(long*)a == *(long*)b;
}

bool uint_eq(void *a, void *b) {
    return *(unsigned int*)a==*(unsigned int*)b;
}

bool ptr_eq(void *a, void *b) {
    return a == b;
}