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

void init_hashtable(hashtable* ht, size_t (*hash)(const char*));
void hashtable_insert(hashtable* ht,const char* key,void* data);
void* hashtable_remove(hashtable* ht,const char* key);
void* hashtable_get(hashtable ht,const char* key);
void hashtable_resize(hashtable* ht, size_t newsize);
size_t hash(const char* str);
void free_hashtable(hashtable* ht);


#endif