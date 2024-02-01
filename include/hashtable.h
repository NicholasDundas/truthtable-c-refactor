#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <string.h>
#include "list.h"

typedef struct {
    list* entries;
    size_t (*hash)(const char*);
    size_t size;
    size_t entry_len;
} hashtable;

//initializes hashtable
//size = 0, entry_len = 0, entries = NULL
void init_hashtable(hashtable* ht, size_t (*hash)(const char*));

//Will either insert or replaces the value depending on whether it exists
//returns 0 on insert, 1 on replace, -1 on failure (failure to allocate memory and sets errno to ENOMEM)
int hashtable_insert(hashtable* ht,const char* key,void* data);

//Removes data from hashtable ht
//returns either NULL or the data removed the table
void* hashtable_remove(hashtable* ht,const char* key);

//attempts to retrieve data in the hashtable with the key provided
//returns either NULL or data in the table
void* hashtable_get(hashtable ht,const char* key);

//resizes the capacity of the hashtable entry 
//returns 0 on success, 1 on failure
int hashtable_resize(hashtable* ht, size_t newsize);

//turns a string of characters into a number
size_t hash(const char* str);

//frees hashtable
void free_hashtable(hashtable* ht);


#endif
