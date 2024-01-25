#include "maybe.h"

#include <stdlib.h>

void* reset_maybe(maybe *m)
{
    m->does_exist = false;
    return m->data; 
}

maybe* free_maybe(maybe *m)
{
    if(m->does_exist) {
        free(reset_maybe(m->data));
    }
    return m;
}

maybe *init_maybe()
{
    maybe* tmp = malloc(sizeof(maybe));
    tmp->does_exist = false;
    return tmp;
}

maybe *init_maybe_wdata(void *data)
{
    maybe* tmp = init_maybe();
    set_maybe(tmp,data);
    return tmp;
}

bool maybe_exists(const maybe* m)
{
    return m->does_exist;
}

void *get_data(const maybe* m)
{
    return m->does_exist ? m->data : NULL;
}

void* set_maybe(maybe *m, void *data) {
    if(maybe_exists(m)) {
        free(m->data);
    }
    m->does_exist = true;
    m->data = data;
    return data;
}
