#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

FILE* open_file(const char* f_name, const char* mode){
    FILE* res = fopen(f_name, mode);
    if (res){
        return res;
    }
    else {
        printf("Файл \"%s\" не удалось открыть.\n",f_name);
        // нужна обработка ошибок
        abort();
    }
}

void close_file(FILE* file){
    if (file != NULL){
        fclose(file);
    }
}

void* create_arr(size_t size){
    void* res = (void*) malloc(size);
    if (!res){
        abort();
        // сделать обработку
    }
    else {
        return res;
    }
}

void check_cap(void** arr, int* cap, int len){
    if (len > (*cap)){
        //printf("oh no len is more than cap\n");

        void* temp = (void*) realloc(*arr, (*cap) * 2);
        (*cap) = (*cap) * 2;
        //printf("new cap is %d\n",*cap);

        if (!temp){
            abort();
            // сделать обработку
        }
        else {
            *arr = (char*)temp;
        }
    }
}

void free_but_better(void* ptr){
    if (ptr){
        free(ptr);
    }
}

void fprintf_state(FILE* file, State state){
    switch (state){
        case STATE_INT:
            fprintf(file, "INT");
            break;
        case STATE_DOUBLE:
            fprintf(file, "DOUBLE");
            break;
        case STATE_STRING:
            fprintf(file, "STRING");
            break;
    }
}