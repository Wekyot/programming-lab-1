#pragma once

#include <stdio.h>
#include "readtable.h"

#define BASE_ARR_SIZE 128
#define LEN_BLOCK 128

FILE* open_file(const char* f_name, const char* mode);
void* create_arr(size_t size);
void check_cap(void** arr, int* cap, int len);
void close_file(FILE* file);

void free_but_better(void* ptr);

void fprintf_state(FILE* file, State state);