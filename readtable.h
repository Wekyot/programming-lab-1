#pragma once

#include <stdio.h>

typedef enum {
    STATE_NULL,
    STATE_SIGN,
    STATE_INT,
    STATE_DOT,
    STATE_DOUBLE,
    STATE_STRING
} State;

enum {
    END_BY_COMMA,
    END_BY_LINE,
    END_BY_ERROR_QUOTE,
    END_BY_ERROR_OVERFLOW
};

#include <stdint.h>

typedef struct {
    uint64_t low;
    int64_t high;
} Int128;

typedef struct {
    char* name;
    State column_type;

    struct{
        long long min;
        long long max;
        int even;
        Int128 integer_sum;
    } INT;
    struct{
        double min;
        double max;
        long double mean;
        int positive;
        int negative;
        int integer;
        double double_sum;
    } DOUBLE;
    struct{
        int nulls;
        int valid_count;    //в сумме с nulls дает общее число строк, нужно для вычисления скользящего среднего
        int min_length;
        int max_length;
        double mean_length;
        int empty;
    } STRING;
} Table_Column;

typedef struct {
    Table_Column* columns;
    int column_count;
    int row_count;
} Table;

int read_one_line(FILE* input, char** buffer, int* cap);
State field_state(char* buf);
int parse_one_field(const char** src, char** buf, int* cap, State* initial_state);