#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdbool.h>

//stores data and pointer to the next node
typedef struct list_node {
    void* data;
    struct list_node* next;
} list_node;

//stores data and information about the list
typedef struct {
    size_t size;
    list_node* head;
    list_node* back;
} list;

//creates and sets list variables
//size = 0, head = NULL
void init_list(list* l);

//adds data to front of list
//returns 0 on success and 1 on failure
int list_front_insert(list* l, void* data);

//adds data to back of list
//returns 0 on success and 1 on failure
int list_back_insert(list* l, void* data);

//adds data to front of list
//returns 0 on success and 1 on failure
int list_insert(list* l, void* data,size_t index);

//sets l[index]->data at index to data
//returns 0 on success and 1 on failure
int list_set(list* l, void* data, size_t index);

//adds data to list via a sorting function
int list_condition_insert(list* l,void* data , bool (*cmprfunc)(void *, void *));

//removes data from front of list and pops
void* list_pop_front(list* l);

//removes data from list at index and pops
void* list_pop_index(list* l,size_t index);

//does a simple memcmpr to check for equality
//returns NULL on false else returns list_node* object
list_node* list_indexof_mem(list l,void* obj,size_t size);

//does a simple memcmpr to check for equality
//returns NULL on false else returns list_node* object
list_node* list_indexof_cmpr(list l,void* obj,bool (*cmprfunc)(void *, void *));

//frees all nodes
//DOES NOT FREE DATA
void free_list(list* list);

//gets data at the index from the list
list_node* list_indexof(list list, size_t index);


//gets data at the index from the list and converts to type
#define list_indexof_cast(TYPE,LIST,INDEX) (*(TYPE*)list_indexof(LIST,INDEX))
#endif
