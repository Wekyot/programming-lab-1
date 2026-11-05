#pragma once

#include "utils.h"
#include "readtable.h"

#include <float.h>

typedef void (*CellHandler)(Table_Column* col, const char* field_buf, State field_t);
// указатель на функцию, обрабатывающую текущее поле

void parse_args(int argc, char* argv[]);
void schema(char* f_name);
void stats(char* f_name);
void report(char* f_name, char* out_f_name);

void parse_csv_stream(const char* f_name, Table* table, CellHandler handle_cell);

void handle_cell_for_schema(Table_Column* col, const char* field_buf, State field_t);
void handle_cell_for_stats(Table_Column* col, const char* field_buf, State field_t);

void init_column_stats(Table_Column* col, const char* header_name);
void free_table_resources(Table* table);

void print_schema_output(const Table* table);
void print_stats_output(const Table* table);
void print_report_output(char* in_f, char* out_f, const Table* table);