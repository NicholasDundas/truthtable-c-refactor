#include "list.h"

#include <errno.h>
#include <string.h>

void init_list(list* l) {
    l->size = 0;
    l->head = NULL;
    l->back = NULL;
}

list_node* init_lnode(list_node* next, void* data) {
    list_node* temp_node = malloc(sizeof(list_node));
    if(temp_node) {
        temp_node->data = data;
        temp_node->next = next;
    } else {
        errno = ENOMEM;
    }
    return temp_node;
}

int list_front_insert(list* l, void* data) {
    list_node* temp_node = init_lnode(l->head, data);
    if(!temp_node) {
        return 1;
    }

    l->head = temp_node;
    if(l->back == NULL) {
        l->back = l->head;
    }
    ++l->size;
    return 0;
}

int list_insert(list *l, void *data, size_t index) {
    if(index == 0) {
        return list_front_insert(l,data);
    } else if (index == l->size) {
        return list_back_insert(l,data);
    } else if (index < l->size) {
        list_node* temp_node = init_lnode(NULL, data);
        if(!temp_node) return 1;

        list_node* traverse = l->head; 
        while(--index) {
            traverse = traverse->next;
        }
        temp_node->next = traverse->next;
        traverse->next = temp_node;
        if(traverse == l->back) {
            l->back = temp_node;
        }
        ++l->size;
        return 0;
    }
    return 1;
}

int list_set(list *l, void *data, size_t index) {
    if(l->size != 0 && index < l->size) {
        list_node* transverse = l->head;
        while(index--) {
            transverse = transverse->next;
        }
        transverse->data = data;
        return 0;
    }
    return 1;
}

int list_back_insert(list *l, void *data) {
    if(l->size == 0) {
        return list_front_insert(l, data);
    } else {
        l->head->next = init_lnode(NULL, data);
        if(!l->head->next) {
            return 1;
        }

        l->back = l->head->next;
        l->size++;
        return 0;
    }
}

int list_condition_insert(list *l, void *data, bool (*cmprfunc)(void *, void *)) {
    if(l->size == 0) {
        return list_front_insert(l,data);
    } else {
        list_node* temp_node = init_lnode(NULL, data);
        if(!temp_node) {
            return 1;
        }

        list_node* traverse = l->head; 
        while(traverse->next && cmprfunc(data,traverse->data)) {
            traverse = traverse->next;
        }
        temp_node->next = traverse->next;
        traverse->next = temp_node;
        return 0;
    }
}

void* list_pop_front(list* l) {
    if (l->size == 0) {
        return NULL;
    }
    list_node* node_to_remove = l->head;
    void* data = node_to_remove->data;
    l->head = node_to_remove->next;
    free(node_to_remove);
    --l->size;
    return data;
}

void* list_pop_index(list* l,size_t index) {
    if (l->size == 0 || index > l->size) {
        return NULL;
    }
    if(index == 0) {
        return list_pop_front(l);
    } else {
        list_node* prev = NULL;
        list_node* cur = l->head;
        while(index-- != 0) {
            prev = cur;
            cur = cur->next;
        }
        void* data = cur->data;
        prev->next = cur->next;
        if(l->back == cur) {
            l->back = prev;
        }
        free(cur);
        --l->size;
        return data;
    }
}

list_node_info list_indexof_mem(list l, void *obj, size_t size) {   
    list_node_info lni = {.node = l.head, .index = 0};
    while(lni.node && memcmp(lni.node->data,obj,size)) {
        lni.node = lni.node->next;
        ++lni.index;
    }
    return lni;
}

list_node_info list_indexof_cmpr(list l, void *obj, bool (*cmprfunc)(void *, void *)) {
    list_node_info lni = {.node = l.head, .index = 0};
    while(lni.node && !cmprfunc(obj,lni.node->data)) {
        lni.node = lni.node->next;
        ++lni.index;
    }
    return lni;
}

void free_list(list* l) {
    if(l) {
        list_node* node = l->head;
        while (--l->size) {
            list_node* next_node = node->next;
            free(node);
            node = next_node;
        }
        l->head = NULL;
    }
}

list_node* list_indexof(list list, size_t index) {
    if (list.size == 0 || index > list.size) {
        return NULL;
    } else if (index == list.size) {
        return list.back->data;
    } else {
        list_node* cur = list.head;
        while(index-- != 0) {
            cur = cur->next;
        }
        return cur;
    }
}
