#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

//stores data and pointer to the next node
typedef struct list_node list_node;


struct list_node {
    void* data;
    list_node* next;
};

//stores data and information about the list
typedef struct {
    size_t size;
    list_node* head;
} list;

//creates and sets list variables
//size = 0, head = NULL
list* init_list();

//adds data to front of list
void add_to_list(list* list, void* data);

//removes data from front of list and pops
void* remove_from_list(list* list);

//removes data from list at index and pops
void* remove_from_list_index(list* list,size_t index);

//does a simple memcmpr to check for equality
//returns SIZE_MAX on false else returns index
size_t list_contains_mem(list* list,void* obj,size_t size);

//frees list object and all nodes
//DOES NOT FREE DATA
void free_list(list* list);

//gets data at the index from the list
void* get_list(list* list, size_t index);

#endif
