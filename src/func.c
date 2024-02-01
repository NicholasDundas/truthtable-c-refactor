#include "func.h"

#include <string.h>

//int equality
bool int_eq(void *a, void *b) {
    return *(int*)a == *(int*)b;
}

//boolean equality
bool bool_eq(void *a, void *b) {
    return *(bool*)a == *(bool*)b;
}

//size_t equality
bool size_t_eq(void *a, void *b) {
    return *(size_t*)a == *(size_t*)b;
}

//char equality
bool char_eq(void *a, void *b) {
    return *(char*)a == *(char*)b;
}

//string equality
bool str_eq(void *a, void *b) {
    return strcmp((char*)a,(char*)b) == 0;
}

//long equality
bool long_eq(void *a, void *b) {
    return *(long*)a == *(long*)b;
}

//unsigned int equality
bool uint_eq(void *a, void *b) {
    return *(unsigned int*)a==*(unsigned int*)b;
}

//pointer equality
bool ptr_eq(void *a, void *b) {
    return a == b;
}