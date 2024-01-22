#include "list.h"

#include <string.h>

list* init_list() {
    list* tmp = malloc(sizeof(list));
    tmp->size = 0;
    tmp->head = NULL;
    return tmp;
}

void add_to_list(list* list, void* data) {
    list_node* temp_node = malloc(sizeof(list_node));
    temp_node->data = data;
    temp_node->next = list->head;
    list->head = temp_node;
    list->size++;
}

void add_to_list_sort(list *list, void *data, bool (*cmprfunc)(void *, void *)) {
    if(list->size == 0) {
        add_to_list(list,data);
    } else {
        list_node* temp_node = malloc(sizeof(list_node));
        temp_node->data = data;
        temp_node->next = NULL;
        list_node* traverse = list->head; 
        size_t index = 0;
        while(++index <= list->size && cmprfunc(data,traverse->data)) {
            traverse = traverse->next;
        }
        temp_node->next = traverse->next;
        traverse->next = temp_node;
    }
}

void add_to_list_index(list *list, void *data, size_t index) {
    if(index == 0) {
        add_to_list(list,data);
    } else if(index <= list->size) {
        list_node* temp_node = malloc(sizeof(list_node));
        temp_node->data = data;
        temp_node->next = NULL;
        list_node* traverse = list->head; 
        while(--index) {
            traverse = traverse->next;
        }
        temp_node->next = traverse->next;
        traverse->next = temp_node;
    }
}

void* remove_from_list(list* list) {
    if (list->size == 0) {
        return NULL;
    }
    list_node* node_to_remove = list->head;
    void* data = node_to_remove->data;
    list->head = node_to_remove->next;
    free(node_to_remove);
    --list->size;
    return data;
}

void* remove_from_list_index(list* list,size_t index) {
    if (list->size == 0 || index > list->size) {
        return NULL;
    }
    list_node* prev = NULL;
    list_node* cur = list->head;
    while(index-- != 0) {
        prev = cur;
        cur = cur->next;
    }
    void* data = cur->data;
    if(prev) {
        prev->next = cur->next;
    } else {
        list->head = cur->next;
    }
    free(cur);
    --list->size;
    return data;
}

size_t list_contains_mem(list *list, void *obj, size_t size)
{   int index = 0;
    list_node* node = list->head;
    while(node != NULL) {
        if(memcmp(node->data,obj,size) == 0)  {
            return index;
        }
        node = node->next;
        ++index;
    }
    return SIZE_MAX;
}

void free_list(list* list) {
    list_node* node = list->head;
    while (node != NULL) {
        list_node* next_node = node->next;
        free(node);
        node = next_node;
    }
    free(list);
}

void *get_list_func(list *list, size_t index)
{
    if (list->size == 0 || index > list->size) {
        return NULL;
    }
    list_node* cur = list->head;
    while(index-- != 0) {
        cur = cur->next;
    }
    return cur->data;
}
