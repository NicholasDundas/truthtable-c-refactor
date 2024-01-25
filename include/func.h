#ifndef FUNC_H
#define FUNC_H
//primitive type compares
#include <stdbool.h>

bool int_eq(void *a,void *b);
bool bool_eq(void *a,void *b);
bool size_t_eq(void *a,void *b);
bool char_eq(void *a,void *b);
bool str_eq(void *a,void *b);
bool ptr_eq(void *a,void *b);
bool uint_eq(void *a, void *b);
bool long_eq(void *a, void *b);

#endif