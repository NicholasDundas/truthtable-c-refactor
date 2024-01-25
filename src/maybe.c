#include "maybe.h"

#include <stdlib.h>

void* reset_maybe(maybe *m) {
    m->does_exist = false;
    return m->data; 
}

void init_maybe(maybe* tmp) {
    tmp->does_exist = false;
}

void* init_maybe_wdata(maybe *tmp, void *data) {
    init_maybe(tmp);
    return set_maybe(tmp,data);
}

bool maybe_exists(maybe m) {
    return m.does_exist;
}

void *get_data(maybe m) {
    return m.does_exist ? m.data : NULL;
}

void* set_maybe(maybe *m, void *data) {
    m->does_exist = true;
    m->data = data;
    return data;
}
