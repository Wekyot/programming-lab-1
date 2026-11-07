#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

FILE* open_file(const char* f_name, const char* mode){
    FILE* res = fopen(f_name, mode);
    if (res){
        return res;
    }
    else {
        fprintf(stderr, "Файл \"%s\" не удалось открыть.\n",f_name);
        return NULL;
    }
}

void free_but_better(void* ptr){
    if (ptr){
        free(ptr);
    }
}

char* state_to_str(State state){
    switch (state){
        case STATE_NULL:
            return "NULL";
            
        case STATE_INT:
            return "INT";
            
        case STATE_DOUBLE:
            return "DOUBLE";
            
        case STATE_STRING:
            return "STRING";

        default:
            return "UNKNOWN";
    }
}

void print_column_name(FILE* stream, const char* name){
    fprintf(stream, "\"");
    while (*name) {
        if (*name == '"') {
            fprintf(stream, "\"\""); 
        } else {
            fputc(*name, stream);
        }
        name++;
    }
    fprintf(stream, "\" ");
}

int is_valid_long_long(const char* buf){
    char* endptr;

    errno = 0;

    strtoll(buf, &endptr, 10);

    if (errno == ERANGE || endptr == buf || *endptr != '\0'){
        return 0;
    }
    
    return 1;
}

int is_valid_double(const char* buf){
    char* endptr;

    errno = 0;

    double val = strtod(buf, &endptr);

    if (errno == ERANGE && (val == HUGE_VAL || val == -HUGE_VAL) || endptr == buf || *endptr != '\0') {
        return 0;
    }
    
    if (!isfinite(val)) {
        return 0;
    }
    
    return 1;
}

int len_of_int(long long int n){
    int len = 0;
    unsigned long long value;

    if (n < 0) {
        len++;
        value = 0ULL - (unsigned long long)n;
    }
    else {
        value = (unsigned long long)n;
    }

    do {
        len++;
        value /= 10;
    } while (value != 0);

    return len;
}

void print_char_line(FILE* f, char c, int count, int need_enter){
    for (int i = 0;i < count;i++)
        fputc(c, f);

    if (need_enter){
        fputc('\n',f);
    }
}

void print_table(FILE* f, const int* widths){
    fputc('+',f);
    for (int i = 0; i < 10;i++){
        print_char_line(f, '-', widths[i], 0);
        fprintf(f, "+");
    }
    fputc('\n',f);
}