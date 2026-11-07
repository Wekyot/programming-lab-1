#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "args_work.h"

static int compare_long_long_double(long long i, double d) {
    if (d < -0x1p63) {
        return 1;
    }

    if (d >= 0x1p63) {
        return -1;
    }

    long long whole = (long long)d;

    if (i < whole) {
        return -1;
    }
    if (i > whole) {
        return 1;
    }

    double fraction = d - (double)whole;

    if (fraction > 0.0) {
        return -1;
    }
    if (fraction < 0.0) {
        return 1;
    }

    return 0;
}

static void print_numeric_extreme(const Table_Column* col, int find_min, int with_decimal) {
    int has_int = col->INT.min <= col->INT.max;
    int has_double = col->DOUBLE.min <= col->DOUBLE.max;

    int use_int;

    if (!has_double) {
        use_int = 1;
    }
    else if (!has_int) {
        use_int = 0;
    }
    else {
        long long int_value = find_min ? col->INT.min : col->INT.max;
        double double_value = find_min ? col->DOUBLE.min : col->DOUBLE.max;

        int comparison = compare_long_long_double(int_value, double_value);

        use_int = find_min ? comparison <= 0 : comparison >= 0;
    }

    if (use_int) {
        if (with_decimal) {
            printf("%lld.000000", find_min ? col->INT.min : col->INT.max);
        }
        else {
            printf("%lld", find_min ? col->INT.min : col->INT.max);
        }
    }
    else {
        printf("%.6lf", find_min ? col->DOUBLE.min : col->DOUBLE.max);
    }
}

static void add_int128(Int128* sum, long long value) {
    uint64_t old_low = sum->low;

    sum->low += (uint64_t)value;
    sum->high += (value < 0 ? -1 : 0);

    if (sum->low < old_low) {
        sum->high++;
    }
}

static double int128_to_double(Int128 sum) {
    int negative = sum.high < 0;

    if (negative) {
        sum.low = ~sum.low + 1;
        sum.high = ~sum.high + (sum.low == 0);
    }

    double result =
        (double)sum.high * 18446744073709551616.0 +
        (double)sum.low;

    return negative ? -result : result;
}

static Int128 divide_uint128(Int128 value, uint32_t divisor, uint32_t* remainder) {
    Int128 quotient = {0, 0};
    uint64_t rem = 0;

    for (int i = 127; i >= 0; i--) {
        uint64_t bit;

        if (i >= 64) {
            bit = (value.high >> (i - 64)) & 1ULL;
        } else {
            bit = (value.low >> i) & 1ULL;
        }

        rem = rem * 2 + bit;

        if (rem >= divisor) {
            rem -= divisor;

            if (i >= 64) {
                quotient.high |= (int64_t)(1ULL << (i - 64));
            } else {
                quotient.low |= 1ULL << i;
            }
        }
    }

    *remainder = (uint32_t)rem;
    return quotient;
}

static void print_integer_mean(Int128 sum, int count) {
    if (count <= 0) {
        printf("0.000000");
        return;
    }

    int negative = sum.high < 0;

    if (negative) {
        sum.low = ~sum.low + 1;
        sum.high = (int64_t)(~(uint64_t)sum.high + (sum.low == 0));
    }

    uint32_t remainder;
    Int128 quotient = divide_uint128(sum, (uint32_t)count, &remainder);

    unsigned long long whole = (unsigned long long)quotient.low;

    uint64_t fractional =
        ((uint64_t)remainder * 1000000 + (uint32_t)count / 2)
        / (uint32_t)count;

    if (fractional == 1000000) {
        whole++;
        fractional = 0;
    }

    if (negative && (whole != 0 || fractional != 0)) {
        printf("-");
    }

    printf("%llu.%06llu", whole,
           (unsigned long long)fractional);
}

static void format_integer_mean(
    char* buffer,
    size_t size,
    Int128 sum,
    int count
) {
    if (count <= 0) {
        snprintf(buffer, size, "0.000000");
        return;
    }

    int negative = sum.high < 0;

    if (negative) {
        sum.low = ~sum.low + 1;
        sum.high = (int64_t)(~(uint64_t)sum.high + (sum.low == 0));
    }

    uint32_t remainder;
    Int128 quotient = divide_uint128(
        sum,
        (uint32_t)count,
        &remainder
    );

    unsigned long long whole = (unsigned long long)quotient.low;

    uint64_t fractional =
        ((uint64_t)remainder * 1000000 + (uint32_t)count / 2)
        / (uint32_t)count;

    if (fractional == 1000000) {
        whole++;
        fractional = 0;
    }

    snprintf(
        buffer,
        size,
        "%s%llu.%06llu",
        negative && (whole != 0 || fractional != 0) ? "-" : "",
        whole,
        (unsigned long long)fractional
    );
}

static void format_numeric_extreme(
    char* buffer,
    size_t size,
    const Table_Column* col,
    int find_min
) {
    int has_int = col->INT.min <= col->INT.max;
    int has_double = col->DOUBLE.min <= col->DOUBLE.max;

    int use_int;

    if (!has_double) {
        use_int = 1;
    }
    else if (!has_int) {
        use_int = 0;
    }
    else {
        long long int_value = find_min
            ? col->INT.min
            : col->INT.max;

        double double_value = find_min
            ? col->DOUBLE.min
            : col->DOUBLE.max;

        int comparison = compare_long_long_double(
            int_value,
            double_value
        );

        use_int = find_min
            ? comparison <= 0
            : comparison >= 0;
    }

    if (use_int) {
        snprintf(
            buffer,
            size,
            col->column_type == STATE_DOUBLE ? "%lld.000000" : "%lld",
            find_min ? col->INT.min : col->INT.max
        );
    }
    else {
        snprintf(
            buffer,
            size,
            "%.6f",
            find_min ? col->DOUBLE.min : col->DOUBLE.max
        );
    }
}

static void format_report_cell(
    char* buffer,
    size_t size,
    const Table_Column* col,
    int column
) {
    switch (column) {
        case REPORT_NAME:
            snprintf(buffer, size, "%s", col->name);
            break;

        case REPORT_TYPE:
            snprintf(buffer, size, "%s", state_to_str(col->column_type));
            break;

        case REPORT_NULLS:
            snprintf(buffer, size, "%d", col->STRING.nulls);
            break;

        case REPORT_VALID:
            snprintf(buffer, size, "%d", col->STRING.valid_count);
            break;

        case REPORT_MIN:
        case REPORT_MAX:
            if (col->column_type == STATE_NULL) {
                snprintf(buffer, size, "N/A");
            }
            else if (col->column_type == STATE_STRING) {
                snprintf(
                    buffer,
                    size,
                    "%d",
                    column == REPORT_MIN
                        ? col->STRING.min_length
                        : col->STRING.max_length
                );
            }
            else {
                format_numeric_extreme(
                    buffer,
                    size,
                    col,
                    column == REPORT_MIN
                );
            }
            break;

        case REPORT_MEAN:
            if (col->column_type == STATE_NULL) {
                snprintf(buffer, size, "N/A");
            }
            else if (col->column_type == STATE_STRING) {
                snprintf(
                    buffer,
                    size,
                    "%.6f",
                    col->STRING.mean_length
                );
            }
            else if (col->column_type == STATE_INT) {
                format_integer_mean(
                    buffer,
                    size,
                    col->INT.integer_sum,
                    col->STRING.valid_count
                );
            }
            else {
                double mean = (
                    int128_to_double(col->INT.integer_sum)
                    + col->DOUBLE.double_sum
                ) / col->STRING.valid_count;

                snprintf(buffer, size, "%.6f", mean);
            }
            break;

        case REPORT_POSITIVE:
            if (col->column_type == STATE_INT ||
                col->column_type == STATE_DOUBLE) {
                snprintf(buffer, size, "%d", col->DOUBLE.positive);
            }
            else {
                snprintf(buffer, size, "N/A");
            }
            break;

        case REPORT_NEGATIVE:
            if (col->column_type == STATE_INT ||
                col->column_type == STATE_DOUBLE) {
                snprintf(buffer, size, "%d", col->DOUBLE.negative);
            }
            else {
                snprintf(buffer, size, "N/A");
            }
            break;
    }
}

int parse_args(int argc, char* argv[]){
    if (argc < 2){
        fprintf(stderr, "Вы запустили команду без аргументов. Попробуйте csvstat --help\n");
        return FUNC_RETURN_ARGS_ERROR;
    }

    if (!strncmp(argv[1],"--help",7)){
        if (argc == 2) {
            printf("Вы запросили справку программы csvstat.\nschema <csv_file_name> [options] выводит статистику по таблице: число строк данных, число столбцов, имя и тип каждого столбца в изначальном порядке.\nФормат вывода:\nrows <number>\ncolumns <number>\n<index> ""<name>"" <type>\n<index> ""<name>"" <type>\n...\n\nСпасибо, что пользуетесь нашей программой!\n\n");
            return FUNC_RETURN_SUCCESS;
        }
        else {
            fprintf(stderr,"Лишний аргумент(ы) при использовании команды --help\n");
            return FUNC_RETURN_ARGS_ERROR;
        }
    }
    else if (!strncmp(argv[1],"schema",7)){
        if (argc == 3){
            return schema(argv[2]);
        }
        else {
            fprintf(stderr,"Недостаточно/слишком много аргументов при использовании команды schema\nОжидалось: csvstat schema <название файла>\n");
            return FUNC_RETURN_ARGS_ERROR;
        }
    }
    else if (!strncmp(argv[1],"stats",6)){
        if (argc == 3){
            return stats(argv[2]);
        }
        else {
            fprintf(stderr,"Недостаточно/слишком много аргументов при использовании команды stats\nОжидалось: csvstat stats <название файла>\n");
            return FUNC_RETURN_ARGS_ERROR;
        }

    }
    else if (!strncmp(argv[1],"report",7)){
        if (argc == 4){
            return report(argv[2],argv[3]);
        }
        else {
            fprintf(stderr,"Недостаточно/слишком много аргументов при использовании команды report\n");
            return FUNC_RETURN_ARGS_ERROR;
        }
    }
    else {
        char pretty_similar_command[7];
        if (strstr(argv[1],"--help")){
            strcpy(pretty_similar_command,"--help");
        }
        else if (strstr(argv[1],"schema")){
            strcpy(pretty_similar_command,"schema");
        }
        else if (strstr(argv[1],"stats")){
            strcpy(pretty_similar_command,"stats");
        }
        else if (strstr(argv[1],"report")){
            strcpy(pretty_similar_command,"report");
        }
        else {
            fprintf(stderr, "Некорректный синтаксис: неизвестная команда.\n");
            return FUNC_RETURN_ARGS_ERROR;
        }

        fprintf(stderr, "Некорректный синтаксис. Возможно вы имели в виду %s?\n",pretty_similar_command);
        return FUNC_RETURN_ARGS_ERROR;
    }
}

int schema(char* f_name){
    Table table = { .column_count = 0, .row_count = 0, .columns = malloc(sizeof(Table_Column) * 256) };
    if (!table.columns){
        return FUNC_RETURN_RUNTIME_ERROR;
    }

    int result = parse_csv_stream(f_name, &table, handle_cell_for_schema);

    if (result == FUNC_RETURN_SUCCESS){
        print_schema_output(&table);
    }
    
    free_table_resources(&table);
    return result;
}

int stats(char* f_name){
    Table table = { .column_count = 0, .row_count = 0, .columns = malloc(sizeof(Table_Column) * 256) };
    if (!table.columns){
        return FUNC_RETURN_RUNTIME_ERROR;
    }

    int result = parse_csv_stream(f_name, &table, handle_cell_for_stats);

    if (result == FUNC_RETURN_SUCCESS){
        print_stats_output(&table);
    }
    
    free_table_resources(&table);

    return result;
}

int report(char* f_name, char* out_f_name){
    Table table = { .column_count = 0, .row_count = 0, .columns = malloc(sizeof(Table_Column) * 256) };
    if (!table.columns){
        return FUNC_RETURN_RUNTIME_ERROR;
    }

    int result = parse_csv_stream(f_name, &table, handle_cell_for_stats);

    if (result == FUNC_RETURN_SUCCESS){
        result = print_report_output(f_name, out_f_name, &table);
    }
    
    free_table_resources(&table);

    return result;
}

int parse_csv_stream(const char* f_name, Table* table, CellHandler handle_cell){
    int return_reason = FUNC_RETURN_SUCCESS;

    char* buffer = NULL;
    char* field_buf = NULL;

    FILE* input = fopen(f_name,"r");
    if (!input){
        return_reason = FUNC_RETURN_RUNTIME_ERROR;
        goto cleanup;
    }
    int cap = BASE_ARR_SIZE;
    buffer = (char*) malloc(sizeof(char) * cap);
    if (!buffer){
        return_reason = FUNC_RETURN_RUNTIME_ERROR;
        goto cleanup;
    }
    if (read_one_line(input, &buffer, &cap) != 1){
        fprintf(stderr, "Файл пуст или произошла ошибка чтения\n");
        return_reason = FUNC_RETURN_RUNTIME_ERROR;
        goto cleanup;
    }
    int field_cap = BASE_ARR_SIZE;
    field_buf = (char*) malloc(sizeof(char) * field_cap);
    if (!field_buf){
        return_reason = FUNC_RETURN_RUNTIME_ERROR;
        goto cleanup;
    }

    const char* parser_ptr = buffer;
    while(1){
        int reason = parse_one_field(&parser_ptr, &field_buf, &field_cap, NULL);

        if (reason == END_BY_ERROR_OVERFLOW) {
            fprintf(stderr, "Ошибка выделения памяти при парсинге заголовка\n");
            return_reason = FUNC_RETURN_RUNTIME_ERROR;
            goto cleanup;
        }

        if (reason == END_BY_ERROR_QUOTE) {
            fprintf(stderr, "Ошибка расстановки кавычек в заголовке %d\n", table->column_count);
            return_reason = FUNC_RETURN_RUNTIME_ERROR;
            goto cleanup;
        }

        if (strlen(field_buf) == 0){
            fprintf(stderr, "Пустое имя столбца %d\n",table->column_count + 1);
            return_reason = FUNC_RETURN_RUNTIME_ERROR;
            goto cleanup;
        }

        for (int i = 0; i < table->column_count; i++) {
            if (strcmp(table->columns[i].name, field_buf) == 0) {
                fprintf(stderr, "Ошибка: Дублирование имени столбца \"%s\" (столбцы %d и %d)\n", field_buf, i, table->column_count);
                return_reason = FUNC_RETURN_RUNTIME_ERROR;
                goto cleanup;
            }
        }

        init_column_stats(&(table->columns[table->column_count]), field_buf);
        if (!table->columns[table->column_count].name){
            fprintf(stderr, "Ошибка выделения памяти при создании имени столбца\n");
            return_reason = FUNC_RETURN_RUNTIME_ERROR;
            goto cleanup;
        }

        table->column_count++;

        if (reason == END_BY_LINE){
            break;
        }
    }

    if (table->column_count < 256){
        Table_Column* temp = realloc(table->columns, sizeof(Table_Column) * table->column_count);
        if (temp){
            table->columns = temp;
        }
        else {
            fprintf(stderr, "Ошибка выделения памяти при создании таблицы\n");
            return_reason = FUNC_RETURN_RUNTIME_ERROR;
            goto cleanup;
        }
    }

    while (1){
        int outer_reason = read_one_line(input, &buffer, &cap);
        if (outer_reason == FUNC_RETURN_RUNTIME_ERROR){
            return_reason = FUNC_RETURN_RUNTIME_ERROR;
            goto cleanup;
        }
        else if (outer_reason == 0){
            break;
        }

        table->row_count++;

        parser_ptr = buffer;
        int column_id = 0;

        while(1){
            State cur_field_state = STATE_NULL;
            int reason = parse_one_field(&parser_ptr, &field_buf, &field_cap, &cur_field_state);

            if (reason == END_BY_ERROR_OVERFLOW){
                fprintf(stderr, "Ошибка выделения памяти при парсинге поля в строке %d столбца %d\n", table->row_count, column_id);
                return_reason = FUNC_RETURN_RUNTIME_ERROR;
                goto cleanup;
            }
            else if (reason == END_BY_ERROR_QUOTE){
                fprintf(stderr, "Ошибка в расстановке кавычек при парсинге поля \"%s\" в строке %d столбца %d\n", field_buf, table->row_count, column_id);
                return_reason = FUNC_RETURN_RUNTIME_ERROR;
                goto cleanup;
            }

            if (column_id >= table->column_count){
                fprintf(stderr, "Ошибка в синтаксисе таблицы файла %s: в строке %d лишние столбцы.\n",f_name,table->row_count);
                return_reason = FUNC_RETURN_RUNTIME_ERROR;
                goto cleanup;
            }

            if (cur_field_state == STATE_NULL){
                cur_field_state = field_state(field_buf);
            }

            handle_cell(&(table->columns[column_id]), field_buf, cur_field_state);
            
            column_id++;
            if (reason == END_BY_LINE){
                break;
            }
        }

        if (column_id < table->column_count){
                fprintf(stderr, "Ошибка в синтаксисе таблицы файла %s: в строке %d недостаточно столбцов.\n",f_name,table->row_count);
                return_reason = FUNC_RETURN_RUNTIME_ERROR;
                goto cleanup;
        }
    }

cleanup:    

    free_but_better(buffer);
    free_but_better(field_buf);
    if (input)
        fclose(input);

    return return_reason;
}

void handle_cell_for_schema(Table_Column* col, const char* field_buf, State field_t){
    (void) field_buf;   //нам не нужно это поле

    if (field_t > (*col).column_type){
        (*col).column_type = field_t;
    }
}

void handle_cell_for_stats(Table_Column* col, const char* field_buf, State field_t){
    if (field_t > (*col).column_type){
        (*col).column_type = field_t;
    }

    // считаем заранее для правильного скользящего среднего
    if (field_t == STATE_NULL){
        col->STRING.nulls++;
        return;
    }
    else {
        col->STRING.valid_count++;
    }

    char* end_ptr;
    switch (field_t){
        case STATE_INT:
            long long int num = strtoll(field_buf, &end_ptr, 10);
            if (num < col->INT.min) {
                col->INT.min = num;
            }
            if (num > col->INT.max) {
                col->INT.max = num;
            }
            if (num % 2 == 0){
                (*col).INT.even++;
            }

            add_int128(&col->INT.integer_sum, num);
            // проваливаемся

        case STATE_DOUBLE:
            long double val = field_t == STATE_INT
                ? (long double)strtoll(field_buf, NULL, 10)
                : (long double)strtod(field_buf, NULL);

            if (field_t == STATE_DOUBLE){
                if (val > (*col).DOUBLE.max){
                    (*col).DOUBLE.max = val;
                }
                if (val < (*col).DOUBLE.min){
                    (*col).DOUBLE.min = val;
                }

                col->DOUBLE.double_sum += val;
            }

            (*col).DOUBLE.mean = (*col).DOUBLE.mean + (val - (*col).DOUBLE.mean)/col->STRING.valid_count;

            if (val == floor(val)){
                col->DOUBLE.integer++;
            }

            if (val > 0.0){
                col->DOUBLE.positive++;
            }
            else if (val < 0.0){
                col->DOUBLE.negative++;
            }

            // проваливаемся

        case STATE_STRING:
            int cur_len = strlen(field_buf);

            if (cur_len > col->STRING.max_length){
                col->STRING.max_length = cur_len;
            }
            if (cur_len < col->STRING.min_length){
                col->STRING.min_length = cur_len;
            }

            col->STRING.mean_length += ((double) cur_len - col->STRING.mean_length)/col->STRING.valid_count;

            if (cur_len == 0){
                col->STRING.empty++;
            }

            break;
    }
}

void init_column_stats(Table_Column* col, const char* header_name){
    col->name = strdup(header_name);
    col->column_type = STATE_NULL;

    col->INT.max = LLONG_MIN;
    col->INT.min = LLONG_MAX;
    col->INT.even = 0;
    col->INT.integer_sum.high = 0;
    col->INT.integer_sum.low = 0;

    col->DOUBLE.integer = 0;
    col->DOUBLE.max = -DBL_MAX;
    col->DOUBLE.min = DBL_MAX;
    col->DOUBLE.mean = 0.0;
    col->DOUBLE.positive = 0;
    col->DOUBLE.negative = 0;
    col->DOUBLE.double_sum = 0;

    col->STRING.empty = 0;
    col->STRING.max_length = 0;
    col->STRING.min_length = INT_MAX;
    col->STRING.mean_length = 0.0;
    col->STRING.nulls = 0;
    col->STRING.valid_count = 0;
}

void free_table_resources(Table* table){
    for (int i = 0;i < table->column_count;i++){
        free_but_better(table->columns[i].name);
    }
    free_but_better(table->columns);
}

void print_schema_output(const Table* table){
    printf("rows %d\ncolumns %d\n", table->row_count, table->column_count);
    for (int i = 0; i < table->column_count;i++){
        printf("%d ",i);
        print_column_name(stdout, table->columns[i].name);
        printf("%s\n",state_to_str(table->columns[i].column_type));
    }
}

void print_stats_output(const Table* table){
    printf("rows %d\ncolumns %d\n\n", table->row_count, table->column_count);

    for (int i = 0;i < table->column_count;i++){
        printf("column %d\nname ",i);
        print_column_name(stdout, table->columns[i].name);
        printf("\ntype %s",state_to_str(table->columns[i].column_type));
        printf("\nnulls %d\n",table->columns[i].STRING.nulls);
        switch (table->columns[i].column_type){
            case STATE_NULL:
                printf("rows %d\n",table->row_count);
                break;
            
            case STATE_INT:
                printf("min ");
                print_numeric_extreme(&table->columns[i], 1, 0);

                printf("\nmax ");
                print_numeric_extreme(&table->columns[i], 0, 0);

                printf("\nmean ");
                print_integer_mean(
                    table->columns[i].INT.integer_sum,
                    table->columns[i].STRING.valid_count
                );

                printf("\npositive %d\nnegative %d\neven %d\n",
                    table->columns[i].DOUBLE.positive,
                    table->columns[i].DOUBLE.negative,
                    table->columns[i].INT.even);
                break;

            case STATE_DOUBLE:
                printf("min ");
                print_numeric_extreme(&table->columns[i], 1, 1);

                printf("\nmax ");
                print_numeric_extreme(&table->columns[i], 0, 1);

                double mean = (int128_to_double(table->columns[i].INT.integer_sum) + table->columns[i].DOUBLE.double_sum) / table->columns[i].STRING.valid_count;

                printf("\nmean %.6lf", mean);

                printf("\npositive %d\nnegative %d\ninteger %d\n",
                    table->columns[i].DOUBLE.positive,
                    table->columns[i].DOUBLE.negative,
                    table->columns[i].DOUBLE.integer);
                break;
            
            case STATE_STRING:
                printf("min_length %d\nmax_length %d\nmean_length %.6lf\nempty %d\n",table->columns[i].STRING.min_length, table->columns[i].STRING.max_length, table->columns[i].STRING.mean_length, table->columns[i].STRING.empty);
                break;
        }
        printf("\n");
    }
}

static void print_report_border(
    FILE* out,
    const int* widths
) {
    fputc('+', out);

    for (int i = 0; i < REPORT_COLUMN_COUNT; i++) {
        print_char_line(out, '-', widths[i] + 2, 0);
        fputc('+', out);
    }

    fputc('\n', out);
}

static void print_report_row(
    FILE* out,
    const char* cells[REPORT_COLUMN_COUNT],
    const int* widths
) {
    fputc('|', out);

    for (int i = 0; i < REPORT_COLUMN_COUNT; i++) {
        if (i == REPORT_NAME || i == REPORT_TYPE) {
            fprintf(out, " %-*s |", widths[i], cells[i]);
        }
        else {
            fprintf(out, " %*s |", widths[i], cells[i]);
        }
    }

    fputc('\n', out);
}

int print_report_output(char* in_f, char* out_f, const Table* table){
    FILE* out = fopen(out_f, "w");
    if (!out){
        return FUNC_RETURN_RUNTIME_ERROR;
    }

    const char* headers[REPORT_COLUMN_COUNT] = {
        "Index",
        "Name",
        "Type",
        "Nulls",
        "Valid",
        "Min",
        "Max",
        "Mean",
        "Positive",
        "Negative"
    };

    int widths[REPORT_COLUMN_COUNT];

    for (int i = 0; i < REPORT_COLUMN_COUNT; i++) {
        widths[i] = strlen(headers[i]);
    }

    int max_len = BASE_ARR_SIZE * 8192;
    char* cell = malloc(max_len);
    if (!cell){
        return FUNC_RETURN_RUNTIME_ERROR;
    }

    for (int i = 0; i < table->column_count; i++) {
        snprintf(cell, max_len, "%d", i);
        widths[REPORT_INDEX] =
            __max(widths[REPORT_INDEX], strlen(cell));

        for (int j = REPORT_NAME; j < REPORT_COLUMN_COUNT; j++) {
            if (j == REPORT_NAME) {
                widths[j] = __max(
                    widths[j],
                    strlen(table->columns[i].name)
                );
                continue;
            }
            
            format_report_cell(
                cell,
                sizeof(cell),
                &table->columns[i],
                j
            );

            widths[j] = __max(widths[j], strlen(cell));
        }
    }

    fprintf(out, "CSV ANALYSIS REPORT\n\n");
    fprintf(out, "File: %s\n", in_f);
    fprintf(out, "Rows: %d\n", table->row_count);
    fprintf(out, "Columns: %d\n\n", table->column_count);

    fprintf(out, "Column summary:\n\n");

    print_report_border(out, widths);
    print_report_row(out, headers, widths);
    print_report_border(out, widths);

    for (int i = 0; i < table->column_count; i++) {
        char formatted[REPORT_COLUMN_COUNT][512];
        const char* cells[REPORT_COLUMN_COUNT];

        snprintf(
            formatted[REPORT_INDEX],
            sizeof(formatted[REPORT_INDEX]),
            "%d",
            i
        );

        cells[REPORT_INDEX] = formatted[REPORT_INDEX];

        for (int j = REPORT_NAME; j < REPORT_COLUMN_COUNT; j++) {
            if (j == REPORT_NAME) {
                cells[j] = table->columns[i].name;
            }
            else {
                format_report_cell(
                    formatted[j],
                    sizeof(formatted[j]),
                    &table->columns[i],
                    j
                );

                cells[j] = formatted[j];
            }
        }

        print_report_row(out, cells, widths);
    }

    print_report_border(out, widths);

    fprintf(out, "\nAdditional metrics:\n\n");

    for (int i = 0; i < table->column_count; i++) {
        const Table_Column* col = &table->columns[i];

        fprintf(out, "Column %d ", i);
        print_column_name(out, col->name);
        fprintf(out, ":\n");

        switch (col->column_type) {
            case STATE_NULL:
                fprintf(out, "Empty columns: %d\n", table->row_count);
                break;

            case STATE_INT:
                fprintf(out, "Even values: %d\n", col->INT.even);
                break;

            case STATE_DOUBLE:
                fprintf(out, "Integer-valued numbers: %d\n", col->DOUBLE.integer);
                break;

            case STATE_STRING:
                fprintf(out, "Empty strings: %d\n", col->STRING.empty);
                break;
        }

        fprintf(out, "\n");
    }

    if (ferror(out)) {
        fclose(out);
        return FUNC_RETURN_RUNTIME_ERROR;
    }

    fclose(out);
    free_but_better(cell);

    return FUNC_RETURN_SUCCESS;
}