#pragma once

#include <stdio.h>
#include "readtable.h"
#include <errno.h>
#include <math.h>

enum {
    FUNC_RETURN_SUCCESS,
    FUNC_RETURN_RUNTIME_ERROR = 2,
    FUNC_RETURN_ARGS_ERROR = 3
};

#define our_max(a,b) (((a) > (b)) ? (a) : (b));

#define BASE_ARR_SIZE 128
#define LEN_BLOCK 128

void check_cap(void** arr, int* cap, int len);
void close_file(FILE* file);

void free_but_better(void* ptr);

void fprintf_state(FILE* file, State state);
void print_column_name(FILE* stream, const char* name);

int is_valid_double(const char* buf);
int is_valid_long_long(const char* buf);

int len_of_int(long long int n);
void print_char_line(FILE* f, char c, int count, int need_enter);
void print_table(FILE* f, const int* widths);

char* state_to_str(State state);