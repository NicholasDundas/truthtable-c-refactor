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

//format specifer : buffer (char*), line (size_t), char position (size_t)
const char* eof_str = "ERROR: Unexpected end of file.\nBUFFER:\"%s\"\nLINE:%zu\nPOSITION:%zu\n";

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
        return 0;
    }
    return 1;
}

inline void cir_declared_at(circuit* cir,size_t line,size_t pos) {
    cir->line_declared = line;
    cir->pos_declared = pos;
}

inline void cir_defined_at(circuit* cir,size_t line,size_t pos) {
    cir->line_defined= line;
    cir->pos_defined = pos;
}

int init_circuit(circuit* cir,const char* name,hashtable* newdict) {
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
        cir->inherited_dictionary = newdict;

        //Inserting Constants into Circuit
        if(!cir->name || !cir->variables || !cir->gates || 
        insert_var(cir, CONST, "0", var_false) ||
        insert_var(cir, CONST, "1", var_true) || 
        insert_var(cir, DISCARD, "_", var_uneval)) {
            free_circuit(cir);
            return 1;
        }

        memcpy(cir->name,name,namelen+1);
    }
    return 0;
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
        return 0;
    }
    return 1;
}

void in_circuit(circuit *cir, size_t in) {
    reset_circuit(cir);
    for (size_t k = 0; k < cir->input_len; k++) {
        cir->variables[k + CIRCUIT_CONST_OFFSET]->value = in & 1;
        in >>= 1;
    }
}

circuit *get_circuit_reference(circuit* cir, char *name)
{
    circuit* tmp = hashtable_get(cir->circuit_dictionary,name);
    if(!tmp) tmp = hashtable_get(*cir->inherited_dictionary,name);
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
    size_t buflen = 0;
    parse_helper ph;
    init_ph(&ph);
    circuit* cir = malloc(sizeof(circuit));
    init_circuit(cir,"MAIN",NULL);
    if(!parse_statement(finput,&buf,&buflen,&ph,cir)) {
        free_circuit(cir);
        cir = NULL;
    }
    free(buf);
    return cir;
}

//same as readbuf_string but prints an end of file error if encountered
inline int readbuf_string_peof(FILE* file,char** pbuf,size_t* pbufsize,parse_helper* ph) {
    const char* eof_str = "ERROR: Unexpected end of file.\nBUFFER:\"%s\"\nLINE:%zu\nPOSITION:%zu\n";
    int res = readbuf_string(file,pbuf,pbufsize,ph);
    if(feof(file)) {
        fprintf(stderr,eof_str,(*pbuf),ph->line,(ph->wrd_end - *pbufsize));
    }
    return res;
}

inline int readbuf_uint_peof(FILE* file,char** pbuf,size_t* pnum,size_t* pbufsize,parse_helper* ph) {
    int res = readbuf_uint(file,pbuf,pnum,pbufsize,ph);
    if(feof(file)) {
        fprintf(stderr,eof_str,(*pbuf),ph->line,(ph->wrd_end - *pbufsize));
    }
    return res;
}

inline bool str_reserved(const char* str) {
    //Names reserved for our use
    const char* reserved_names[] = {"CIR", "CONST", "AND", "NAND", "OR", "NOR","XOR", "DECODER","NOT","MULTIPLEXER","PASS","INPUT", "OUTPUT", "_","1","0", "END"};
    for(size_t i = 0; i < sizeof(reserved_names)/sizeof(char*); ++i) {
        if(strcmp(*str,reserved_names[i]) == 0) {
            return true;
        }
    }
    return false;
}

//Attempts to create a circuit header
//Should check if the circuit needs to be defined afterwards
//Returns NULL if it fails
//Failure Conditions include
//Circuit Already Defined
//Duplicate Declaration in Scope
circuit* read_cir_header(FILE* finput, char** buf, size_t* buflen, parse_helper* ph, circuit* outer_cir) {
    errno = 0;
    circuit* newcir = NULL;
    const parse_helper funcstart = *ph;

    if(readbuf_string_peof(finput,buf,buflen,ph) < 1) {
        return NULL;
    }

    if(str_reserved(*buf)) {
        fprintf(stderr,"ERROR: \"%s\" cannot be used as a name as it is reserved\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->wrd_end - *buflen);
        return NULL;
    }

    //New circuit or does it exist (and where is it)
    newcir = get_circuit_reference(outer_cir,*buf);
    if(newcir) {
        if(newcir->type == cir_defined) {
            fprintf(stderr,"ERROR: \"%s\" cannot be used as a name as it is already defined\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->wrd_end - *buflen);
            return NULL;
        }
    } else {
        newcir = malloc(sizeof(circuit));
        if(!newcir || 
        init_circuit(newcir,*buf,&outer_cir->circuit_dictionary)) {
            errno = ENOMEM;
            return NULL;
        }
    }

    //Grab INPUT identifier
    if(readbuf_string_peof(finput,buf,buflen,ph) < 1) {
        goto CIR_CLEANUP;
    }
    if(strcmp(*buf,"INPUT") != 0) {
        fprintf(stderr,"ERROR: \"%s\" found when INPUT identifier expected \nLINE:%zu\nPOSITION:%zu\n",*buf,ph->line,ph->wrd_end - *buflen);
        goto CIR_CLEANUP;
    }

    //num of inputs....
    size_t inputnum;
    if(readbuf_uint_peof(finput,buf,&inputnum,buflen,ph) < 1) {
        goto CIR_CLEANUP;
    }
    if(newcir->type == cir_declared && inputnum != newcir->input_len) {
        fprintf(stderr,"ERROR: Circuit declaration mismatch\nFUNC \'%s\' INPUT \'%zu\' OUTPUT \'%zu\' does not match input length found : \"%s\" \nLINE:%zu\nPOSITION:%zu\n",newcir->name,newcir->input_len,newcir->output_len,(*buf),ph->line,ph->wrd_end - *buflen);
        goto CIR_CLEANUP;
    } else if (inputnum > MAX_VAR_COUNT) {
        fprintf(stderr,"ERROR: \"%s\" exceeds max variable length\nLINE:%zu\nPOSITION:%zu\n",*buf,ph->line,ph->wrd_end - *buflen);
        goto CIR_CLEANUP;
    }
    newcir->input_len = inputnum;

    //Reading Output Identifier
    if(readbuf_string_peof(finput,buf,buflen,ph) < 1) {
        goto CIR_CLEANUP;
    }
    if(strcmp(*buf,"OUTPUT") == 0) {
        if (declared_in_scope(*outer_cir,newcir->name)) {
            fprintf(stderr,"ERROR: \"%s\" cannot be used as a name as it is declared already as a circuit on line: %zu, position %zu\nLINE:%zu\nPOSITION:%zu\n",*buf,newcir->line_declared,newcir->pos_declared,ph->line,ph->wrd_end - *buflen);
            goto CIR_CLEANUP;
        }
READOUTPUT: 
        if(readbuf_uint_peof(finput,buf,&inputnum,buflen,ph) < 1) {
            goto CIR_CLEANUP;
        }
        if(newcir->type == cir_declared && inputnum != newcir->output_len) {
            fprintf(stderr,"ERROR: Circuit function declaration mismatch\nFUNC \'%s\' INPUT \'%zu\' OUTPUT \'%zu\' does not match output length previously declared : \"%s\" \nLINE:%zu\nPOSITION:%zu\n",newcir->name,newcir->input_len,newcir->output_len,(*buf),ph->line,ph->wrd_end - *buflen);
            goto CIR_CLEANUP;
        } else if (inputnum > MAX_VAR_COUNT) {
            fprintf(stderr,"ERROR: \"%s\" exceeds max variable length\nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->wrd_end - *buflen);
            goto CIR_CLEANUP;
        }
        newcir->output_len = inputnum;
        if(newcir->type == cir_init) { //Function Declaration Specialization (if we have read no input variables then we are expecting to return a declaration)
            newcir->type = cir_declared;
            newcir->pos_declared = funcstart.pos;
            return newcir;
        }
    }
    //read in input or output vars
    while(inputnum--) {
        if(str_reserved(*buf)) {
            fprintf(stderr,"ERROR: \"%s\" cannot be used as a name as it is reserved\nLINE:%zu\nPOSITION:%zu\n",*buf,ph->line,ph->wrd_end - *buflen);
            goto CIR_CLEANUP;
        }
        if(declared_in_scope(*outer_cir,*buf)) {
            fprintf(stderr,"ERROR: \"%s\" cannot be used as a name as it is declared already as a circuit on line: %zu, position %zu\nLINE:%zu\nPOSITION:%zu\n",*buf,newcir->line_declared,newcir->pos_declared,ph->line,ph->wrd_end - *buflen);
            goto CIR_CLEANUP;
        }        
        if(get_var(newcir,*buf) != newcir->var_len) {
            fprintf(stderr,"ERROR: \"%s\" cannot be used as a name as it is already declared as a variable previously\nLINE:%zu\nPOSITION:%zu\n",*buf,ph->line,ph->wrd_end - *buflen);
            goto CIR_CLEANUP;
        }
        if(!insert_var(newcir,INPUT,*buf,var_uneval)) {
            goto CIR_CLEANUP;
        }
        if(readbuf_string_peof(finput,buf,buflen,ph) < 1) {
            goto CIR_CLEANUP;
        }
    }

    if(newcir->type == cir_init) {
        newcir->type = cir_defined;
        if(readbuf_string_peof(finput,buf,buflen,ph) < 1) {
            goto CIR_CLEANUP;
        }
        if(strcmp(*buf,"OUTPUT") != 0) {
            fprintf(stderr,"ERROR: \"%s\" found when OUTPUT identifier expected \nLINE:%zu\nPOSITION:%zu\n",(*buf),ph->line,ph->wrd_end - *buflen);
            goto CIR_CLEANUP;
        }
        goto READOUTPUT;
    } 
    return newcir;
    
CIR_CLEANUP:
    free_circuit(newcir);
    return NULL;
}


int parse_statement(FILE* finput, char** buf,size_t* buflen,parse_helper*ph,circuit* c) {
    if(strcmp(*buf,"CIR") == 0) { //We are parsing a Function Declaration or Definition
        parse_helper func_ph = *ph;
        circuit* newcir = read_cir_header(finput,buf,buflen,ph,c);
        if(!newcir) { //error reading header...
            fprintf(stderr,"CIRCUIT HEADER ERROR\nLINE:%zu\nPOS:%zu\n",func_ph.line,(func_ph.wrd_end - strlen("FUNC")));
            return false;
        }
        if(!hashtable_insert(&c->circuit_dictionary,newcir->name,newcir)) { //malloc failure?
            fprintf(stderr,"CIRCUIT DICTIONARY INSERTION ERROR\nFUNC NAME:'%s'\nLINE:%zu\n",newcir->name,func_ph.line);
            return false;
        }
        if(newcir->type == cir_defined) { //defined circuit
            while(parse_statement(finput,buf,buflen,ph,newcir)) {}
            if(strcmp(*buf,"END") != 0) {
                if(feof(finput)) {
                    fprintf(stderr,eof_str,(*buf),ph->line,ph->wrd_end - *buflen);
                }
                return false;
            }
            if(!valid_circuit(*newcir)) {
                fprintf(stderr,"INVALID CIRCUIT ERROR\nFUNC NAME:'%s'\nLINE:%zu\n",newcir->name,func_ph.line);
            }
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
                tmp->pos = ph->wrd_end-strlen(buf)-1;
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
                    tmp->pos = ph->wrd_end-strlen(buf)-1;
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