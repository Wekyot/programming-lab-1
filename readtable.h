#pragma once

#include <stdio.h>

typedef enum {
    STATE_NONE,
    STATE_SIGN,
    STATE_INT,
    STATE_DOT,
    STATE_DOUBLE,
    STATE_STRING,
    STATE_EMPTY
} State;

enum {
    END_BY_COMMA,
    END_BY_LINE
};

typedef struct {
    char* name;
    State column_type;

    union {
        struct{
            int nulls;
            long long min;
            long long max;
            long long mean;
            int positive;
            int negative;
            int even;
        } INT;
        struct{
            int nulls;
            long long min;
            long long max;
            long long mean;
            int positive;
            int negative;
            int integer;
        } DOUBLE;
        struct{
            int nulls;
            long long min_length;
            long long max_length;
            long long mean_length;
            int empty;
        } STRING;
    };
} Table_Column;

typedef struct {
    Table_Column* columns;
    int column_count;
    int row_count;
} Table;

int read_one_line(FILE* input, char** buffer, int* cap);
State field_state(char* buf);
int parse_one_field(const char** src, char* buf, size_t max_len, int is_header);