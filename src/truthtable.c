/*
* author: Nicholas Dundas
* date: 1/5/2024
* Truthtable program I have refactored from an older code made in my Sophmore year of College
* Takes a file as the first argument and prints out a truth table.
*
* Simple usage
* .\truthtable <FILE> 
*
*/



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

#include "circuit.h"




int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage : %s <FILE_NAME>\n", argv[0]);
        return EXIT_FAILURE;
    }
    time_t start = time(NULL);
    FILE* output = stdout;
    circuit* cir = read_from_file(argv[1]);
    if(cir != NULL) {
        for (size_t i = 0; i < (size_t)1 << cir->input_len; ++i) {
            //Setting Inputs to an increasing Number i
            in_circuit(cir, i);
            out_circuit(cir);
            for (size_t k = cir->input_len - 1; k != SIZE_MAX; --k) {
                fprintf(output,"%d ", cir->variables[k + CIRCUIT_CONST_OFFSET]->value);
            }
            fprintf(output,"|");
            for (size_t k = cir->input_len; k < cir->input_len + cir->output_len; ++k) {
                fprintf(output," %d", cir->variables[k + CIRCUIT_CONST_OFFSET]->value);
            }
            fprintf(output,"\n");
        }
        fprintf(output,"\n\n");
        //print_circuit(output,cir);
        free_circuit(cir);
    } else {
        if(output != stderr && output != stdout)
            fclose(output);
        return EXIT_FAILURE;
    }
    if(output != stderr && output != stdout)
            fclose(output);
    fprintf(stdout,"Time taken: %lf\n", difftime(time(NULL), start));
    return EXIT_SUCCESS;
}
