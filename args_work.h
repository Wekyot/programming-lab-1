#pragma once

#include "utils.h"
#include "readtable.h"

void parse_args(int argc, char* argv[]);
void schema(char* f_name);
void update_column_schema(State* column_state, State type);