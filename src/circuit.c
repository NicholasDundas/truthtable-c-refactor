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
#include "parsehelper.h"

size_t get_var(circuit* c, char* name) { 
    if (c == NULL || name == NULL) {
        return GET_VAR_NULL_PASSED;
    }
    for (size_t i = 0; i != c->var_len; ++i)
        if (strncmp(c->variables[i]->letter, name, NAME_SIZE) == 0)
            return i;
    return c->var_len;
}

static size_t min_len(size_t a, size_t b) {
    if(a < b)
        return a;
    else 
        return b;
}

int insert_var(circuit* c, type_t type, char* name, var_result value) {
    if (c != NULL && name != NULL) {
        if (c->var_len >= c->var_cap) {
            c->var_cap *= 2;
            c->variables = realloc(c->variables, sizeof(variable*) * c->var_cap);
        }

        c->variables[c->var_len] = malloc(sizeof(variable));
        size_t namelen = min_len(strlen(name),NAME_SIZE);
        memcpy(c->variables[c->var_len]->letter, name, namelen);
        c->variables[c->var_len]->letter[namelen] = '\0';

        c->variables[c->var_len]->type = type;

        c->variables[c->var_len]->value = value;

        ++c->var_len;
        return 0;
    }
    return INSERT_GATE_FAILURE;
}

circuit* init_circuit(void) {
    circuit* cir = malloc(sizeof(circuit));
    if (cir != NULL) {
        cir->gate_len = 0;
        cir->gate_cap = 2;
        cir->gates = malloc(sizeof(gate) * cir->gate_cap);;

        cir->var_len = 0;
        cir->var_cap = 4;
        cir->variables = malloc(sizeof(variable) * cir->var_cap);
        //Inserting Constants into Circuit
        insert_var(cir, CONST, "0", var_false);
        insert_var(cir, CONST, "1", var_true);
        insert_var(cir, DISCARD, "_", var_uneval);

        cir->input_len = 0;
        cir->output_len = 0;
    }
    return cir;
}

void free_circuit(circuit* cir) {
    if (cir != NULL) {
        for (size_t i = 0; i < cir->var_len; ++i) {
            free(cir->variables[i]); 
        }
        free(cir->variables);
        for (size_t i = 0; i < cir->gate_len; ++i) {
            free(cir->gates[i].params); 
        }
        free(cir->gates);
        while(cir->circuit_dictionary->size != 0) {
            free(remove_from_list(cir->circuit_dictionary));
        }
        free_list(cir->circuit_dictionary);
        free(cir);
    }
}

int insert_gate(circuit* c, kind_t kind, variable** params, size_t size,size_t total_size,circuit* cir) {
    if (c != NULL) {
        if (c->gate_len >= c->gate_cap) {
            c->gate_cap = (c->gate_cap << 1) - (c->gate_cap >> 1);
            c->gates = realloc(c->gates, sizeof(gate) * c->gate_cap);
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

void print_circuit(FILE *file, circuit *cir)
{
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
    for(size_t i = CIRCUIT_CONST_OFFSET; i < cir->var_len; i++) {
        if(cir->variables[i]->type == TEMP) {
                cir->variables[i]->value = var_uneval;
        }
    }
}


size_t out_circuit(circuit *cir) {
    //Preforming Gate Actions
    reset_circuit(cir);
    list* l = init_list();
    for (size_t c = 0; c < cir->gate_len; c++) {
        if(is_evaluable(cir->gates[c])) {
            gate_return(&cir->gates[c]);
        } else {
            add_to_list(l,&cir->gates[c]);
        }
        for(size_t c = l->size-1; c != SIZE_MAX ; --c) {
            gate* index = get_list_func(l,c);
            if(is_evaluable(*index)) {
                gate_return((gate*)remove_from_list_index(l,c));
            }
        }
    }
    free_list(l);
    size_t num = 0;
    for (size_t k = cir->input_len + cir->output_len; k != cir->input_len; k--) {
        num <<= 1;
        num |= cir->variables[(k - 1) + CIRCUIT_CONST_OFFSET]->value & 1;
    }
    return num;
}


typedef struct {
    size_t line;
    size_t pos;
} word_pos;

//Errors READ_VAR can throw
typedef enum { READ_VAR_SUCCESS = 0,OUTPUT_EXPECTED_ERROR = 2, INPUT_EXPECTED_ERROR ,UNKNOWN_ERROR, BUFFER_OVERFLW, READ_VAR_EOF = EOF } READ_VAR_FAIL;

//variable reading helper function
READ_VAR_FAIL read_var(FILE* finput, circuit* cir, char* buf,size_t i,size_t output_cap, variable** value,parse_helper* ph,list* outputs, list* inputs,list* inputs_pos) { 
    int res = readbuf_string(finput, buf, NAME_SIZE,ph);
    if (res == 0)
        return BUFFER_OVERFLW;
    size_t index = get_var(cir, buf);
    //If it doesnt exist (and we need outputs), create it!
    if (index == cir->var_len) {
        insert_var(cir, TEMP, buf,false);
        *value = cir->variables[cir->var_len - 1];
        if(i < output_cap) {
            if((index = list_contains_mem(outputs,*value,sizeof(variable*))) == SIZE_MAX) {
                word_pos* tmp = malloc(sizeof(word_pos));
                tmp->line = ph->line;
                tmp->pos = ph->lastword_pos-strlen(buf)-1;
                add_to_list(inputs_pos,tmp);
                add_to_list(inputs,*value);
            }
        } else {
            add_to_list(outputs,*value);
        }
        return res != EOF ? READ_VAR_SUCCESS : READ_VAR_EOF;
    }
    //Must read in an input, const, or temp as an input before the index i reaches output_cap, in which it will only accept const_out, temp, and OUTPUT
    else if (index != cir->var_len && //Make sure we found something before...
        ((i >= output_cap && output_friendly(*cir->variables[index])) //Are we expecting output?
        || (i < output_cap && input_friendly(*cir->variables[index])))) { //Are we expecting input?
            *value = cir->variables[index];
            if(i < output_cap) {
                if(list_contains_mem(outputs,*value,sizeof(variable*)) == SIZE_MAX 
                && list_contains_mem(inputs,*value,sizeof(variable*)) == SIZE_MAX) {
                    word_pos* tmp = malloc(sizeof(word_pos));
                    tmp->line = ph->line;
                    tmp->pos = ph->lastword_pos-strlen(buf)-1;
                    add_to_list(inputs_pos,tmp);
                    add_to_list(inputs,*value);
                }
            } else {
                add_to_list(outputs,*value);
                while((index = list_contains_mem(inputs,*value,sizeof(variable*))) != SIZE_MAX) {
                    free(remove_from_list_index(inputs_pos,index));
                    remove_from_list_index(inputs,index);
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

bool expect(FILE* file,char* buf,size_t bufmax,const char* expected,size_t expected_size,parse_helper* ph) {
    switch(readbuf_string(file,buf,bufmax,ph)) {
        case 1:
        case EOF:
            if(strncmp(buf,expected,expected_size) == 0) {
                return true;
            } else {
                return false;
            }
        default:
        case 0:
            return false;
    }
}

circuit* read_from_file_name(char *filename) {
    FILE* finput = fopen(filename,"r");
    if (finput == NULL) {
        fprintf(stderr, "error opening file, \"%s\": %s",filename ,strerror(errno));
        return NULL;
    }
    parse_helper* ph = init_ph();
    circuit* cir = read_from_file(finput,ph,false);
    free(ph);
    return cir;
}


cd_entry *create_entry(const char* name, size_t size, circuit* ptr) {
    cd_entry* tmp = malloc(sizeof(cd_entry));
    strncpy(tmp->letter,name,size);
    tmp->ptr = malloc(sizeof(circuit*));
    tmp->ptr = ptr;
    tmp->declared = false;
    tmp->input_len = 0;
    tmp->output_len = 0;
    return tmp;
}

void free_entry(cd_entry* entry) {
    if(entry->ptr != NULL) {
        free_circuit(entry->ptr);
        
    }
    free(entry);
}

circuit* read_from_file(FILE *finput, parse_helper* ph,list* cir_dictionary_param) {
    
    circuit* cir = init_circuit();

    list* expected_input_vars = init_list();
    list* expected_input_vars_pos = init_list();

    list* temp_outputs = init_list();
    for(size_t i = 0;i< cir->var_len;i++) {
        add_to_list(temp_outputs,cir->variables[i]);
    }

    list* circuit_dictionary;
    if(cir_dictionary_param == NULL) {
        circuit_dictionary = init_list();
    }
    else {
        circuit_dictionary = cir_dictionary_param;
    }

    size_t index = GET_VAR_NULL_PASSED;
    char* buf = malloc(NAME_SIZE + 1);
    int c;
    while((c = readbuf_string(finput, buf, NAME_SIZE,ph)) == 1 && strncmp(buf,"FUNC",4) == 0) {
        if(cir_dictionary_param == NULL) { //we are in the top most recursive level
            switch ((c = readbuf_string(finput, buf, NAME_SIZE,ph))) {
                case 1:
                    list_node* tmp = circuit_dictionary->head;
                    while(tmp != NULL && strncmp(((circuit_dictionary_entry*)tmp->data)->letter,buf,NAME_SIZE) != 0) {
                        tmp = tmp->next;
                    }
                    if(tmp != NULL) {
                        fprintf(stderr, "Duplicate Function name found \"%s\"\nLINE:%zu\nPOSITION:%zu",buf,ph->line,ph->lastword_pos-strlen(buf)-2);
                        goto FAIL;
                    }
                    circuit_dictionary_entry* tmpentry = create_entry(buf,NAME_SIZE,malloc(sizeof(circuit*)));
                    tmpentry->ptr = read_from_file(finput,ph,circuit_dictionary);
                    if(finput == NULL)
                        goto FAIL;
                    add_to_list(circuit_dictionary,tmpentry);
                    break;
                case EOF:
                    fprintf(stderr, "End of file encountered when reading FUNC name\nNAME:%s\nLINE:%zu\nPOSITION:%zu",buf,ph->line,ph->lastword_pos-strlen(buf)-2);
                    __attribute__ ((fallthrough));
                case 0:
                    goto FAIL;
                    break;
            }
        } else {
            fprintf(stderr, "Not allowed to declare FUNC within functions\nLINE:%zu\nPOSITION:%zu",buf,ph->line,ph->lastword_pos-strlen(buf)-2);
            goto FAIL;
        }
    }
    switch (c) {
        case EOF:
            fprintf(stderr, "End of file encountered when attempting to read %s variable name\nBUF:%s\nLINE:%zu\nPOSITION:%zu",expectvalueslower[x],buf,ph->line,ph->lastword_pos-strlen(buf)-2);
            __attribute__ ((fallthrough));
        case 0:
            goto FAIL;
        default:
    }
    size_t* cirptr[] = { &cir->input_len, &cir->output_len};
    char* expectvalues[] = {"INPUT", "OUTPUT" };
    char* expectvalueslower[] = {"input", "output"};
    for(size_t x = 0; x < 2; x++) { //read in input/output
        if(strncmp(buf,expectvalues[x],NAME_SIZE) == 0) {
                if (!readbuf_uint(finput, cirptr[x], NAME_SIZE, ph)) {
                    goto FAIL;
                }
                if (cir->input_len > MAX_VAR_COUNT) {
                    fprintf(stderr, "ERROR: %s count, \"%zu\", exceeds MAX_VAR_COUNT, \"%zu\"\nLINE:%zu\nPOSITION:%zu",expectvalueslower[x] ,cir->input_len, MAX_VAR_COUNT,ph->line,ph->lastword_pos-strlen(buf));
                    goto FAIL;
                }
                for (size_t i = 0; i < *cirptr[x]; i++) {
                    if ((c = readbuf_string(finput, buf, NAME_SIZE,ph)) != 1) {
                        switch (c) {
                            case EOF:
                                fprintf(stderr, "End of file encountered when attempting to read %s variable name\nBUF:%s\nLINE:%zu\nPOSITION:%zu",expectvalueslower[x],buf,ph->line,ph->lastword_pos-strlen(buf)-2);
                                __attribute__ ((fallthrough));
                            case 0:
                                goto FAIL;   
                        }
                    }
                    if ((index = get_var(cir, buf)) == cir->var_len) {
                        insert_var(cir, (x == 0 ? INPUT : OUTPUT), buf, false);
                        if(x == 0) {
                            add_to_list(temp_outputs,cir->variables[cir->var_len-1]);
                        } else if(x == 1) {
                            word_pos* tmp = malloc(sizeof(word_pos));
                            tmp->line = ph->line;
                            tmp->pos = ph->lastword_pos-strlen(buf)-1;
                            add_to_list(expected_input_vars_pos,tmp);
                            add_to_list(expected_input_vars,cir->variables[cir->var_len-1]);
                        }
                    }
                    else if (index < cir->var_len) {
                        fprintf(stderr, "duplicate %s variable name entered : %s\nLINE:%zu\nPOSITION:%zu",expectvalueslower[x], buf,ph->line,ph->lastword_pos-strlen(buf)-1);
                        goto FAIL;
                    }
                    else {
                        fprintf(stderr, "invalid %s variable name entered : %s\nLINE:%zu\nPOSITION:%zu",expectvalueslower[x], buf,ph->line,ph->lastword_pos-strlen(buf)-1);
                        goto FAIL;
                    }
            }
        } else {
            goto FAIL;
        }
        if ((c = readbuf_string(finput, buf, NAME_SIZE,ph)) != 1) {
            switch (c) {
                case EOF:
                    fprintf(stderr, "End of file encountered when attempting to find %s identifer\nBUF:%s\nLINE:%zu\nPOSITION:%zu",expectvalueslower[x],buf,ph->line,ph->lastword_pos-strlen(buf)-2);
                    __attribute__ ((fallthrough));
                case 0:
                    goto FAIL;   
            }
        }
    }
    //Read in circuit
    int read_result = 1;
    while ((read_result = readbuf_string(finput, buf, NAME_SIZE,ph)) == 1) {
        gate temp_gate = {.kind = 0, .params = NULL, .size = 0, .total_size = 0, .cir_ptr = NULL};
        if (strncmp(buf, "NOT", 3) == 0) temp_gate.kind = NOT;
        else if (strncmp(buf, "PASS", 4) == 0) temp_gate.kind = PASS;
        else if (strncmp(buf, "AND", 3) == 0) temp_gate.kind = AND;
        else if (strncmp(buf, "OR", 2) == 0) temp_gate.kind = OR;
        else if (strncmp(buf, "NAND", 4) == 0) temp_gate.kind = NAND;
        else if (strncmp(buf, "NOR", 3) == 0) temp_gate.kind = NOR;
        else if (strncmp(buf, "XOR", 3) == 0) temp_gate.kind = XOR;
        else if (strncmp(buf, "DECODER",7) == 0) temp_gate.kind = DECODER;
        else if (strncmp(buf, "MULTIPLEXER",11) == 0) temp_gate.kind = MULTIPLEXER;
        else if (cir_dictionary_param == NULL && strncmp(buf, "FUNC",4) == 0) {
            readbuf_string(finput, buf, NAME_SIZE,ph);
            circuit_pointer* cirp = malloc(sizeof(circuit_pointer));
            strncpy(cirp->letter,buf,NAME_SIZE);
            circuit* new_cir = read_from_file(finput,ph,true);
            cirp->ptr = new_cir;
            if(new_cir == NULL) {
                free(cirp);
                goto FAIL;
            }
            add_to_list(cir->circuit_dictionary,cirp);
        } else if (sub == true && strncmp(buf, "END",3) == 0) {
            goto ENDFUNC;
        }  else {
            list_node* node = cir->circuit_dictionary->head;
            while(node != NULL) {
                circuit_pointer* tmp = node->data;
                if(strncmp(tmp->letter,buf,NAME_SIZE) == 0) {
                    temp_gate.kind = CIR_PTR;
                    temp_gate.cir_ptr = tmp->ptr;
                }
                node = node->next;
            }
            if (node == NULL)  {
                fprintf(stderr, "Unknown gate type : %s\nLINE:%zu\nPOSITION:%zu", buf,ph->line,ph->lastword_pos-strlen(buf)-2);
            }
            goto FAIL;
        }
        temp_gate.size = 1; temp_gate.total_size = 2; //Size of params (includes all inputs, outputs, selectors, etc)
        size_t output_cap = 1; //Index of where outputs may start
        switch (temp_gate.kind) {
            case AND:
            case OR:
            case NAND:
            case NOR:
            case XOR:
                temp_gate.size = 2;
                temp_gate.total_size = 3; //2 inputs params[0,1,...], 1 output.... params[...,2]
                output_cap = 2;
                __attribute__ ((fallthrough));
            case NOT:
            case PASS:
                break;
            case DECODER:
            case MULTIPLEXER:
                readbuf_uint(finput, &temp_gate.size, NAME_SIZE, ph);
                if (temp_gate.size > MAX_VAR_COUNT) {
                    fprintf(stderr, "gate size, \"%zu\", exceeds MAX_VAR_COUNT, \"%zu\"\nLINE:%zu\nPOSITION:%zu", temp_gate.size, MAX_VAR_COUNT,ph->line,ph->lastword_pos-strlen(buf)-1);
                    goto FAIL;
                }
                temp_gate.total_size = temp_gate.size + ((size_t)1 << temp_gate.size) + (temp_gate.kind == MULTIPLEXER ? 1 : 0);
                output_cap = temp_gate.size + (temp_gate.kind == MULTIPLEXER ? ((size_t)1 << temp_gate.size) : 0);
                break;
            case CIR_PTR:
                temp_gate.size = temp_gate.cir_ptr->input_len;
                temp_gate.total_size = temp_gate.cir_ptr->var_len - CIRCUIT_CONST_OFFSET;
                output_cap = temp_gate.cir_ptr->input_len + CIRCUIT_CONST_OFFSET;
        }
        temp_gate.params = malloc(sizeof(variable*) * temp_gate.total_size);
        for (size_t i = 0; i < temp_gate.total_size; ++i) {
            READ_VAR_FAIL res = read_var(finput,cir,buf,i,output_cap, &temp_gate.params[i],ph,temp_outputs,expected_input_vars,expected_input_vars_pos);
            if (temp_gate.params[i] == NULL) {
                switch (res) {
                    case INPUT_EXPECTED_ERROR:
                        fprintf(stderr, "could not read in variable as  \"%s\" is not of type CONST, INPUT, or TEMP\n\nLINE:%zu\nPOSITION:%zu", buf,ph->line,ph->lastword_pos-strlen(buf)-1);
                        break;
                    case OUTPUT_EXPECTED_ERROR:
                        fprintf(stderr, "could not read in variable as  \"%s\" is not of type DISCARD, OUTPUT, or TEMP\n\nLINE:%zu\nPOSITION:%zu", buf,ph->line,ph->lastword_pos-strlen(buf)-1);
                        break;
                    case UNKNOWN_ERROR: 
                    default:
                        break;               
                }
                goto FAIL;
            }
        }
        insert_gate(cir, temp_gate.kind, temp_gate.params, temp_gate.size, temp_gate.total_size,temp_gate.cir_ptr);       
    }
    if (read_result != EOF) {
        fprintf(stderr, "error reading gates, buf = \"%s\"\nLINE:%zu\nPOSITION:%zu",buf,ph->line,ph->lastword_pos-strlen(buf)-1);
        goto FAIL;
    }
    ENDFUNC:
    if(expected_input_vars->size != 0) {
        fprintf(stderr, "Error, missing outputs for\n");
        list_node* node = expected_input_vars->head;
        while(node != NULL) {
            fprintf(stderr,"VARIABLE:%s\nTYPE:%s\nLINE:%zu\nPOSITION:%zu\n",((variable*)node->data)->letter,variable_type_to_char(((variable*)node->data)->type),((word_pos*)expected_input_vars_pos->head->data)->line,((word_pos*)expected_input_vars_pos->head->data)->pos);
            node = node->next;
            free(remove_from_list(expected_input_vars_pos));
        }
        fprintf(stderr,"\n");
        goto FAIL;
    }

    free(buf); 
    free_list(expected_input_vars);
    free_list(temp_outputs);
    free_list(expected_input_vars_pos);
    return cir;
    FAIL: //Failstate exits and returns 1
    while(expected_input_vars_pos->size != 0) {
        free(remove_from_list(expected_input_vars_pos));
    }
    while(cir->circuit_dictionary->size != 0) {
        free(remove_from_list(cir->circuit_dictionary));
    }
    free_circuit(cir);
    cir = NULL;
    free(buf); 
    free_list(expected_input_vars);
    free_list(temp_outputs);
    free_list(expected_input_vars_pos);
    return cir; 
}
