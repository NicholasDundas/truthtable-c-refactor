#include "hashtable.h"

typedef struct {
    char* key;
    void* data;
} hashtable_node;

void init_hashtable(hashtable *tmp,size_t (*hash)(const char *)) {
    tmp->hash = hash;
    tmp->size = 0;
    tmp->entry_len = 8;
    tmp->entries = malloc(sizeof(list) * tmp->entry_len);
    for(size_t i = 0; i < tmp->entry_len; ++i) {
        init_list(&tmp->entries[i]);
    }
}

size_t one_one_half(size_t num) {
    return (num << 1) - (num >> 1);
}
static inline bool hashnode_key_cmpr(void* key, void* htn) {
    return strcmp(key,((hashtable_node*)htn)->key) == 0;
}

static inline bool hashnode_cmpr(void* htn1, void* htn2) {
    return strcmp(((hashtable_node*)htn1)->key,((hashtable_node*)htn2)->key) == 0;
}

void hashtable_insert(hashtable* ht,const char *key,void *data) {
    size_t index;
    list_node* list_index;
    while(index = ht->hash(key) % ht->entry_len,
    (one_one_half(ht->entry_len) >> 1) <= ht->entries[index].size) {
        hashtable_resize(ht,ht->entry_len << 1);
    } 
    if (ht->entries[index].size == 0 && (list_index = list_indexof_cmpr(ht->entries[index],(void*)key,hashnode_key_cmpr))) {
        hashtable_node* tmp = malloc(sizeof(hashtable_node));
        tmp->data = data;
        size_t keylen = strlen(key);
        tmp->key = malloc(keylen+1);
        memcpy(tmp->key,key,keylen);
        tmp->key[keylen] = '\0';
        list_front_insert(&ht->entries[index],tmp);
        ht->size++;
    } else {
        list_index->data = data;
    }
}

static inline void hashtable_insert_node(hashtable* ht,hashtable_node* htn) {
    size_t index;
    list_node* list_index;
    while(index = ht->hash(htn->key) % ht->entry_len,
    (one_one_half(ht->entry_len) >> 1) <= ht->entries[index].size) {
        hashtable_resize(ht,ht->entry_len << 1);
    } 
    if (ht->entries[index].size == 0 && (list_index = list_indexof_cmpr(ht->entries[index],htn,hashnode_cmpr))) {
        list_front_insert(&ht->entries[index],htn);
    } else {
        free(((hashtable_node*)list_index->data)->key);
        free(list_index->data);
        list_index->data = htn;
    }    
}

void hashtable_resize(hashtable *ht, size_t newsize) {
    hashtable tmp;
    tmp.hash = ht->hash;
    tmp.size = 0;
    tmp.entry_len = newsize;
    tmp.entries = malloc(sizeof(list) * newsize); 
    for(size_t i = 0; i < tmp.entry_len; ++i) {
        init_list(&tmp.entries[i]);
    }
    for(size_t i = 0; i < ht->entry_len; ++i) {
        while(ht->entries[i].size) {
            hashtable_insert_node(&tmp,ht->entries[i].head->data);
            list_pop_front(&ht->entries[i]);
        }
    }
    ht->entry_len = newsize;
    ht->entries = tmp.entries;
}

void free_hashtable(hashtable* ht) {
    for(size_t i = 0; i < ht->entry_len; ++i) {
        while(ht->entries[i].size) {
            free(((hashtable_node*)ht->entries[i].head->data)->key);
            free(list_pop_front(&ht->entries[i]));
        }
    }
}
void *hashtable_remove(hashtable *ht,const char *key) {
    size_t index = ht->hash(key) % ht->entry_len;
    list_node* transverse = ht->entries[index].head;
    list_node* prev = NULL;
    int res;
    while(transverse && 
    (res = strcmp(((hashtable_node*)transverse->data)->key,key)) != 0) {
        prev = transverse;
        transverse = transverse->next;
    }
    if(transverse && !res) {
        if(prev) {
            prev->next = transverse->next;
        } else {
            ht->entries[index].head = transverse->next;
        }
        free(((hashtable_node*)transverse->data)->key);
        void* data = ((hashtable_node*)transverse->data)->data;
        free(transverse->data);
        --ht->entries[index].size;
        return data;
    }
    return NULL;
}

void *hashtable_get(hashtable ht,const char *key){
    size_t index = ht.hash(key) % ht.entry_len;
    list_node* transverse = ht.entries[index].head;
    int res;
    while(transverse && 
    (res = strcmp(((hashtable_node*)transverse->data)->key,key)) != 0) {
        transverse = transverse->next;
    }
    if(transverse && !res)
        return ((hashtable_node*)transverse->data)->data;
    return NULL;
}

size_t hash(const char *key)
{
    size_t res = 0;
    while(*key) {
        res += (size_t)(*key++);
    }
    return res;
}