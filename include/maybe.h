#ifndef MAYBE_H
#define MAYBE_H

#include <stdbool.h>

//structure to hold data and whether the data exists
typedef struct {
    bool does_exist;
    void* data;
} maybe;

//sets does_exist to false 
void* reset_maybe(maybe* m);


//creates empty maybe and sets does_exist to false
void init_maybe(maybe* tmp);

//equivalent to init_maybe() followed by set_maybe() with data
void* init_maybe_wdata(maybe* tmp, void* data);

//checks if maybe data exists
bool maybe_exists(maybe m);

//returns data or NULL if does_exist is false
void* get_data(maybe m);

#define get_data_cast(TYPE,maybe) (*(CAST*)get_data(maybe))

//sets data and updates does_exist to true
//returns set data
void* set_maybe(maybe* m, void* data);

#endif
