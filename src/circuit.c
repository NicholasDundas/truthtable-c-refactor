#include "circuit.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include "variable.h"
#include "gate.h"
#include "list.h"
#include "hashtable.h"
#include "parsehelper.h"

size_t get_var(circuit* c, char* name) { 
    if (c == NULL || name == NULL) {
        return GET_VAR_NULL_PASSED;
    }
    for (size_t i = 0; i != c->var_len; ++i)
        if (strcmp(c->variables[i]->letter, name) == 0)
            return i;
    return c->var_len;
}

inline static size_t min_len(size_t a, size_t b) {
    return a < b ? a : b;
}

int insert_var(circuit* c, var_type type, char* name, var_result value) {
    if (c != NULL && name != NULL) {
        if (c->var_len >= c->var_cap) {
            c->var_cap *= 2;
            c->variables = realloc(c->variables, sizeof(variable*) * c->var_cap);
            if(!c->variables) {
                errno = ENOMEM;
                return 0;
            }
        }
        c->variables[c->var_len] = malloc(sizeof(variable));

        size_t namelen = strlen(name);
        c->variables[c->var_len]->letter = malloc(namelen+1);
        memcpy(c->variables[c->var_len]->letter, name,namelen+1);

        c->variables[c->var_len]->type = type;
        c->variables[c->var_len]->value = value;

        ++c->var_len;
        return 1;
    }
    return 0;
}


int init_circuit(circuit* cir, const char* name) {
    if (cir) {
        cir->type = cir_init;

        size_t namelen = strlen(name);
        cir->name = malloc(namelen+1);
        cir->input_len = 0;
        cir->output_len = 0;

        cir->gate_len = 0;
        cir->gate_cap = 2;
        cir->gates = malloc(sizeof(gate) * cir->gate_cap);

        cir->var_len = 0;
        cir->var_cap = 4;
        cir->variables = malloc(sizeof(variable*) * cir->var_cap);

        init_hashtable(&cir->circuit_dictionary,hash) ;
        init_hashtable(&cir->inherited_dictionary,hash);

        //Inserting Constants into Circuit
        if(!cir->name || !cir->variables || !cir->gates || 
        !insert_var(cir, CONST, "0", var_false) ||
        !insert_var(cir, CONST, "1", var_true) || 
        !insert_var(cir, DISCARD, "_", var_uneval)) {
            free_circuit(cir);
            return 0;
        }

        memcpy(cir->name,name,namelen+1);
    }
    return 1;
}

void free_circuit(circuit* cir) {
    if (cir != NULL) {
        while(cir->var_len--) {
            free(cir->variables[cir->var_len]); 
        }
        free(cir->variables);
        cir->variables = NULL;
        while(cir->gate_len--) {
            free(cir->gates[cir->gate_len].params); 
        }
        free(cir->gates);
        cir->gates = NULL;
        free_hashtable(&cir->circuit_dictionary);
        free_hashtable(&cir->inherited_dictionary);
        free(cir->name);
        cir->name = NULL;
        cir->type = cir_freed;
    }
}

int insert_gate(circuit* c, gate_type kind, variable** params, size_t size,size_t total_size,circuit* cir) {
    if (c != NULL) {
        if (c->gate_len >= c->gate_cap) {
            c->gate_cap = (c->gate_cap << 1) - (c->gate_cap >> 1);
            c->gates = realloc(c->gates, sizeof(gate) * c->gate_cap);
            if(!c->gates) {
                errno = ENOMEM;
                return 0;
            }
        }
        c->gates[c->gate_len].kind = kind;
        c->gates[c->gate_len].params = params;
        c->gates[c->gate_len].size = size;
        c->gates[c->gate_len].cir_ptr = cir;
        c->gates[c->gate_len].total_size = total_size;
        ++c->gate_len;
        return 1;
    }
    return 0;
}

void in_circuit(circuit *cir, size_t in) {
    reset_circuit(cir);
    for (size_t k = 0; k < cir->input_len; k++) {
        cir->variables[k + CIRCUIT_CONST_OFFSET]->value = in & 1;
        in >>= 1;
    }
}

circuit *get_circuit_reference(const circuit* cir, char *name)
{
    circuit* tmp = hashtable_get(cir->circuit_dictionary,name);
    if(!tmp) tmp = hashtable_get(cir->inherited_dictionary,name);
    if(!tmp && strcmp(cir->name,name) == 0) tmp = cir;
    return tmp;
}

bool declared_in_scope(circuit cir, char *name) {
    circuit* tmp = hashtable_get(cir.circuit_dictionary,name);
    return tmp ? tmp->type == cir_declared : false;
}

bool declared(circuit cir, char *name) {
    circuit* tmp = get_circuit_reference(&cir,name);
    return tmp ? tmp->type >= cir_declared : false;
}


void print_circuit(FILE *file, circuit *cir) {
    fprintf(file, "CIRCUIT\n VARIABLES:");
    for(size_t i = 0; i < cir->var_len; i++) {
        fprintf(file,"\n  ");
        print_var(file, cir->variables[i]);
    }
    fprintf(file, "\n GATES:");
    for(size_t i = 0; i < cir->gate_len; i++) {
        fprintf(file,"\n  ");
        print_gate(file,cir->gates[i]);
    }
}

void reset_circuit(circuit *cir) {
    for(size_t i = CIRCUIT_CONST_OFFSET + cir->input_len + cir->output_len; i < cir->var_len; ++i) {
        if(cir->variables[i]->type == TEMP) {
                cir->variables[i]->value = var_uneval;
        }
    }
}

size_t out_circuit(circuit *cir) {
    //Preforming Gate Actions
    list l;
    init_list(&l);
    for (size_t c = 0; c < cir->gate_len; c++) {
        if(is_evaluable(cir->gates[c])) {
            gate_return(&cir->gates[c]);
        } else {
            list_front_insert(&l,&cir->gates[c]);
        }
        for(size_t c = l.size-1; c != SIZE_MAX ; --c) {
            gate* index = &list_indexof_cast(gate,l,c);
            if(is_evaluable(*index)) {
                gate_return((gate*)list_pop_index(&l,c));
            }
        }
    }
    free_list(&l);
    size_t num = 0;
    for (size_t k = cir->input_len + cir->output_len; k != cir->input_len; k--) {
        num <<= 1;
        num |= cir->variables[(k - 1) + CIRCUIT_CONST_OFFSET]->value & 1;
    }
    return num;
}

circuit* read_from_file_name(char *filename) {
    FILE* finput = fopen(filename,"r");
    if (finput == NULL) {
        fprintf(stderr, "error opening file, \"%s\": %s",filename ,strerror(errno));
        return NULL;
    }
    char* buf = NULL;
    size_t bufsize = 0;
    parse_helper ph;
    init_ph(&ph);
    circuit* cir = malloc(sizeof(circuit));
    init_circuit(cir, "MAIN");
    if(!parse_statement(finput,&buf,&bufsize,&ph,cir)) {
        free_circuit(cir);
        cir = NULL;
    }
    free(buf);
    return cir;
}

//Attempts to create a function header
//Should check if the function needs to be defined afterwards
//Returns NULL if it fails
circuit* read_func_header(FILE* finput, char** buf, size_t* bufsize, parse_helper* ph, circuit* outer_cir) {
    errno = 0;
    const char* eof_str = "ERROR: Unexpected end of file.\nBUFFER:\"%s\"\nLINE:%zu\nPOSITION:%zu\n";
    circuit* func = NULL;

    int res = readbuf_string(finput,buf,bufsize,ph);
    if(res < 1) {
        if(feof(finput)) {
            fprintf(stderr,eof_str,(*buf),ph->line,ph->lastword_pos - *bufsize);
        }
        return NULL;
    }

    //Names reserved for our use
    char* reserved_names[] = {"FUNC", "CONST", "AND", "NAND", "OR", "NOR","XOR", "DECODER","NOT","MULTIPLEXER","PASS","INPUT", "OUTPUT", "_","1","0"};
    for(size_t i = 0; i < sizeof(reserved_names)/sizeof(char*); ++i) {
        if(strcmp(*buf,reserved_names[i]) == 0) {
            fprintf(stderr,"ERROR: \"%s\" cannot be used as a name as it is reserved\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->lastword_pos - *bufsize);
            return NULL;
        }
    }

    //New circuit or does it exist (and where is it)
    func = get_circuit_reference(outer_cir,buf);
    if(func) {
        if(func->type == cir_defined) {
            fprintf(stderr,"ERROR: \"%s\" cannot be used as a name as it is already defined\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->lastword_pos - *bufsize);
            return NULL;
        } else if (declared_in_scope(*outer_cir,buf)) {
            fprintf(stderr,"ERROR: \"%s\" cannot be used as a name as it is already declared in scope\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->lastword_pos - *bufsize);
            return NULL;
        }
    } else {
        func = malloc(sizeof(circuit));
        if(!func || !init_circuit(func,buf)) {
            errno = ENOMEM;
            return NULL;
        }
        func->inherited_dictionary = outer_cir->circuit_dictionary;
    }

    //Grab INPUT identifier
    res = readbuf_string(finput,buf,bufsize,ph);
    if(res < 1) {
        if(feof(finput)) {
            fprintf(stderr,eof_str,(*buf),ph->line,ph->lastword_pos - *bufsize);
        }
        goto CIR_CLEANUP;
    }
    if(strcmp(buf,"INPUT") != 0) {
        fprintf(stderr,"ERROR: \"%s\" found when INPUT identifier expected \nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->lastword_pos - *bufsize);
        goto CIR_CLEANUP;
    }

    //num of inputs....
    size_t inputnum;
    res = readbuf_uint(finput,buf,&inputnum,bufsize,ph);
    if(res < 1) {
        if(feof(finput)) {
            fprintf(stderr,eof_str,(*buf),ph->line,ph->lastword_pos - *bufsize);
        }
        goto CIR_CLEANUP;
    }
    if(func->type == declared && inputnum != func->input_len) {
        fprintf(stderr,"ERROR: Circuit function declaration mismatch\nFUNC \'%s\' INPUT \'%zu\' OUTPUT \'%zu\' does not match input length found : \"%s\" \nLINE:%zu\nPOSITION:%zu\n",func->name,func->input_len,func->output_len,(*buf),ph->line,ph->lastword_pos - *bufsize);
        goto CIR_CLEANUP;
    } else if (inputnum > MAX_VAR_COUNT) {
        fprintf(stderr,"ERROR: \"%s\" exceeds max variable length\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->lastword_pos - *bufsize);
        goto CIR_CLEANUP;
    }
    func->input_len = inputnum;

    //Reading Output Identifier
    res = readbuf_string(finput,buf,bufsize,ph);
    if(res < 1) {
        if(feof(finput)) {
            fprintf(stderr,eof_str,(*buf),ph->line,ph->lastword_pos - *bufsize);
        }
        goto CIR_CLEANUP;
    }
    if(strcmp(buf,"OUTPUT") == 0) {
READOUTPUT: 
        res = readbuf_uint(finput,buf,&inputnum,bufsize,ph);
        if(res < 1) {
            if(feof(finput)) {
                fprintf(stderr,eof_str,(*buf),ph->line,ph->lastword_pos - *bufsize);
            }
            goto CIR_CLEANUP;
        }
        if(func->type == declared && inputnum != func->output_len) {
            fprintf(stderr,"ERROR: Circuit function declaration mismatch\nFUNC \'%s\' INPUT \'%zu\' OUTPUT \'%zu\' does not match output length previously declared : \"%s\" \nLINE:%zu\nPOSITION:%zu\n",func->name,func->input_len,func->output_len,(*buf),ph->line,ph->lastword_pos - *bufsize);
            goto CIR_CLEANUP;
        } else if (inputnum > MAX_VAR_COUNT) {
            fprintf(stderr,"ERROR: \"%s\" exceeds max variable length\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->lastword_pos - *bufsize);
            goto CIR_CLEANUP;
        }
        func->output_len = inputnum;
        if(func->type == cir_init) { //Function Declaration Specialization (if not init then we are expecting to read vars)
            func->type = cir_declared;
            return func;
        }
    }
    //read in input or output vars
    while(inputnum--) {
        for(size_t i = 0; i < sizeof(reserved_names)/sizeof(char*); ++i) {
            if(strcmp(*buf,reserved_names[i]) == 0) {
                fprintf(stderr,"ERROR: \"%s\" cannot be used as a name as it is reserved\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->lastword_pos - *bufsize);
                goto CIR_CLEANUP;
            }
        }
        if(declared(*outer_cir,*buf)) {
            fprintf(stderr,"ERROR: \"%s\" cannot be used as a name as it is declared already as a circuit function\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->lastword_pos - *bufsize);
            goto CIR_CLEANUP;
        }
        if(!insert_var(func,INPUT,*buf,var_uneval)) {
            goto CIR_CLEANUP;
        }
        if(inputnum) {
            res = readbuf_string(finput,buf,bufsize,ph);
            if(res < 1) {
                if(feof(finput)) {
                    fprintf(stderr,eof_str,(*buf),ph->line,ph->lastword_pos - *bufsize);
                }
                goto CIR_CLEANUP;
            }
        }
    }

    if(func->type = cir_defined)
        return func;

    func->type = cir_defined;
    goto READOUTPUT;

CIR_CLEANUP:
    free_circuit(func);
    return NULL;
}


int parse_logexpr(FILE* finput, char** buf,size_t* bufsize,parse_helper*ph,circuit* c) {

}

int parse_statement(FILE* finput, char** buf,size_t* bufsize,parse_helper*ph,circuit* c) {
    if(strcmp(*buf,"FUNC") == 0) { //We are parsing a Function Declaration or Definition
        parse_helper func_ph = *ph;
        circuit* cir = read_func_header(finput,buf,bufsize,ph,c);
        if(!cir) {
            fprintf(stderr,"CIRCUIT FUNCTION ERROR\nLINE:%zu\n",func_ph.line);
            return false;
        }
        if(!hashtable_insert(&c->circuit_dictionary,cir->name,cir)) {
            fprintf(stderr,"CIRCUIT FUNCTION INSERTION ERROR\nFUNC NAME:'%s'\nLINE:%zu\n",cir->name,func_ph.line);
            return false;
        }
        if(cir->type == cir_defined) {
            int res;
            while(res = parse_logexpr(finput,buf,bufsize,ph,c)) {}
        }
    }
}



/*
typedef struct {
    size_t line;
    size_t pos;
} word_pos;

typedef enum { READ_VAR_SUCCESS = 0,OUTPUT_EXPECTED_ERROR = 2, INPUT_EXPECTED_ERROR ,UNKNOWN_ERROR, BUFFER_OVERFLW, READ_VAR_EOF = EOF } READ_VAR_FAIL;

READ_VAR_FAIL read_var(FILE* finput, circuit* cir, char** buf,size_t i,size_t output_cap, variable** value,list* outputs, list* inputs,list* inputs_pos) { 
    int res = readbuf_string(finput, buf,&i ,ph);
    if (res == 0)
        return BUFFER_OVERFLW;
    size_t index = get_var(cir, buf);
    //If it doesnt exist (and we need outputs), create it!
    if (index == cir->var_len) {
        insert_var(cir, TEMP, buf,false);
        *value = cir->variables[cir->var_len - 1];
        if(i < output_cap) {
            if((index = list_indexof_mem(*outputs,*value,sizeof(variable*))) == SIZE_MAX) {
                word_pos* tmp = malloc(sizeof(word_pos));
                tmp->line = ph->line;
                tmp->pos = ph->lastword_pos-strlen(buf)-1;
                list_front_insert(inputs_pos,tmp);
                list_front_insert(inputs,*value);
            }
        } else {
            list_front_insert(outputs,*value);
        }
        return res != EOF ? READ_VAR_SUCCESS : READ_VAR_EOF;
    }
    //Must read in an input, const, or temp as an input before the index i reaches output_cap, in which it will only accept const_out, temp, and OUTPUT
    else if (index != cir->var_len && //Make sure we found something before...
        ((i >= output_cap && output_friendly(*cir->variables[index])) //Are we expecting output?
        || (i < output_cap && input_friendly(*cir->variables[index])))) { //Are we expecting input?
            *value = cir->variables[index];
            if(i < output_cap) {
                if(list_indexof_mem(*outputs,*value,sizeof(variable*)) == SIZE_MAX 
                && list_indexof_mem(*inputs,*value,sizeof(variable*)) == SIZE_MAX) {
                    word_pos* tmp = malloc(sizeof(word_pos));
                    tmp->line = ph->line;
                    tmp->pos = ph->lastword_pos-strlen(buf)-1;
                    list_front_insert(inputs_pos,tmp);
                    list_front_insert(inputs,*value);
                }
            } else {
                list_front_insert(outputs,*value);
                while((index = list_indexof_mem(*inputs,*value,sizeof(variable*))) != SIZE_MAX) {
                    free(list_pop_index(inputs_pos,index));
                    list_pop_index(inputs,index);
                }
            }
            return res != EOF ? READ_VAR_SUCCESS : READ_VAR_EOF;
    }
    *value = NULL;
    if (i >= output_cap && input_friendly(*cir->variables[index])) {
        return OUTPUT_EXPECTED_ERROR;
    }
    if (i < output_cap && output_friendly(*cir->variables[index])) {
        return INPUT_EXPECTED_ERROR;
    }
    return UNKNOWN_ERROR;
}


*/