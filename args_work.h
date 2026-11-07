#pragma once

#include "utils.h"
#include "readtable.h"

#include <float.h>

enum {
    REPORT_INDEX,
    REPORT_NAME,
    REPORT_TYPE,
    REPORT_NULLS,
    REPORT_VALID,
    REPORT_MIN,
    REPORT_MAX,
    REPORT_MEAN,
    REPORT_POSITIVE,
    REPORT_NEGATIVE,
    REPORT_COLUMN_COUNT
};

typedef void (*CellHandler)(Table_Column* col, const char* field_buf, State field_t);
// указатель на функцию, обрабатывающую текущее поле

int parse_args(int argc, char* argv[]);
int schema(char* f_name);
int stats(char* f_name);
int report(char* f_name, char* out_f_name);

int parse_csv_stream(const char* f_name, Table* table, CellHandler handle_cell);

void handle_cell_for_schema(Table_Column* col, const char* field_buf, State field_t);
void handle_cell_for_stats(Table_Column* col, const char* field_buf, State field_t);

void init_column_stats(Table_Column* col, const char* header_name);
void free_table_resources(Table* table);

void print_schema_output(const Table* table);
void print_stats_output(const Table* table);
int print_report_output(char* in_f, char* out_f, const Table* table);