#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>

#include "args_work.h"

static char* format_alloc(const char* format, ...) {
    va_list args;
    va_start(args, format);

    va_list args_copy;
    va_copy(args_copy, args);

    int length = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if (length < 0) {
        va_end(args_copy);
        return NULL;
    }

    char* buffer = malloc((size_t)length + 1);
    if (!buffer) {
        va_end(args_copy);
        return NULL;
    }

    int result = vsnprintf(buffer, (size_t)length + 1, format, args_copy);
    va_end(args_copy);

    if (result < 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

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

static char* format_integer_mean(Int128 sum, int count) {
    if (count <= 0) {
        return format_alloc("0.000000");
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

    return format_alloc(
        "%s%llu.%06llu",
        negative && (whole != 0 || fractional != 0) ? "-" : "",
        whole,
        (unsigned long long)fractional
    );
}

static char* format_numeric_extreme(
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
        return format_alloc(
            col->column_type == STATE_DOUBLE
                ? "%lld.000000"
                : "%lld",
            find_min ? col->INT.min : col->INT.max
        );
    }

    return format_alloc(
        "%.6f",
        find_min ? col->DOUBLE.min : col->DOUBLE.max
    );
}

static char* format_report_cell(
    const Table_Column* col,
    int column
) {
    switch (column) {
        case REPORT_NAME:
            return format_alloc("%s", col->name);

        case REPORT_TYPE:
            return format_alloc("%s", state_to_str(col->column_type));

        case REPORT_NULLS:
            return format_alloc("%d", col->STRING.nulls);

        case REPORT_VALID:
            return format_alloc("%d", col->STRING.valid_count);

        case REPORT_MIN:
        case REPORT_MAX:
            if (col->column_type == STATE_NULL) {
                return format_alloc("N/A");
            }

            if (col->column_type == STATE_STRING) {
                return format_alloc(
                    "%d",
                    column == REPORT_MIN
                        ? col->STRING.min_length
                        : col->STRING.max_length
                );
            }

            return format_numeric_extreme(
                col,
                column == REPORT_MIN
            );

        case REPORT_MEAN:
            if (col->column_type == STATE_NULL) {
                return format_alloc("N/A");
            }

            if (col->column_type == STATE_STRING) {
                return format_alloc("%.6f", col->STRING.mean_length);
            }

            if (col->column_type == STATE_INT) {
                return format_integer_mean(
                    col->INT.integer_sum,
                    col->STRING.valid_count
                );
            }

            return format_alloc(
                "%.6f",
                (
                    int128_to_double(col->INT.integer_sum)
                    + col->DOUBLE.double_sum
                ) / col->STRING.valid_count
            );

        case REPORT_POSITIVE:
            if (col->column_type == STATE_INT ||
                col->column_type == STATE_DOUBLE) {
                return format_alloc("%d", col->DOUBLE.positive);
            }

            return format_alloc("N/A");

        case REPORT_NEGATIVE:
            if (col->column_type == STATE_INT ||
                col->column_type == STATE_DOUBLE) {
                return format_alloc("%d", col->DOUBLE.negative);
            }

            return format_alloc("N/A");
    }

    return NULL;
}

static const char* report_header(int column) {
    switch (column) {
        case REPORT_INDEX:    return "Index";
        case REPORT_NAME:     return "Name";
        case REPORT_TYPE:     return "Type";
        case REPORT_NULLS:    return "Nulls";
        case REPORT_VALID:    return "Valid";
        case REPORT_MIN:      return "Min";
        case REPORT_MAX:      return "Max";
        case REPORT_MEAN:     return "Mean";
        case REPORT_POSITIVE: return "Positive";
        case REPORT_NEGATIVE: return "Negative";
    }

    return "Unknown";
}

int parse_args(int argc, char* argv[]){
    if (argc < 2){
        fprintf(stderr, "Вы запустили команду без аргументов. Попробуйте csvstat --help\n");
        return FUNC_RETURN_ARGS_ERROR;
    }

    if (!strncmp(argv[1],"--help",7)){
        if (argc == 2) {
            printf(
                "Вы запросили справку программы csvstat.\n\n"
                "Доступные команды:\n\n"
                "schema <csv_file_name> выводит информацию о структуре таблицы: "
                "число строк данных, число столбцов, имя и тип каждого столбца "
                "в изначальном порядке.\n\n"
                "stats <csv_file_name> выводит подробную статистику по каждому столбцу: "
                "число пустых значений, минимальное и максимальное значения, "
                "среднее значение, количество положительных и отрицательных чисел, "
                "а также дополнительные показатели в зависимости от типа столбца.\n\n"
                "report <csv_file_name> <output_file_name> формирует текстовый отчёт "
                "со сводной статистикой по таблице и сохраняет его в указанный файл.\n\n"
                "Формат вывода команды schema:\n"
                "rows <number>\n"
                "columns <number>\n"
                "<index> \"<name>\" <type>\n"
                "<index> \"<name>\" <type>\n"
                "...\n\n"
                "Спасибо, что пользуетесь нашей программой!\n\n"
            );
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
            if (!strcmp(argv[2],argv[3])){
                fprintf(stderr, "Названия входного и выходного файла совпадают. Это перезапишет входной файл. Принудительно завершаюсь\n");
            }
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
        fprintf(stderr, "Файл не удалось открыть или он не найден\n");
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

    int pending_empty_lines = 0;

    while (1) {
        int outer_reason = read_one_line(input, &buffer, &cap);

        if (outer_reason == FUNC_RETURN_RUNTIME_ERROR) {
            return_reason = FUNC_RETURN_RUNTIME_ERROR;
            goto cleanup;
        }
        else if (outer_reason == 0) {
            break;
        }

        if (buffer[0] == '\n' && buffer[1] == '\0') {
            pending_empty_lines++;
            continue;
        }

        if (pending_empty_lines > 0) {
            if (table->column_count > 1) {
                fprintf(stderr, "Ошибка в синтаксисе таблицы файла %s: недостаточно столбцов.\n", f_name);

                return_reason = FUNC_RETURN_RUNTIME_ERROR;
                goto cleanup;
            }

            for (int i = 0; i < pending_empty_lines; i++) {
                table->row_count++;
                handle_cell(
                    &table->columns[0],
                    "",
                    STATE_NULL
                );
            }

            pending_empty_lines = 0;
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

            if (val == 0.0L) {
                val = 0.0L;
            }

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
        printf("\ntype %s\n",state_to_str(table->columns[i].column_type));

        if (table->columns[i].column_type != STATE_NULL)
            printf("nulls %d\n",table->columns[i].STRING.nulls);

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
        if (i != table->column_count - 1)
            printf("\n");
    }
}

static void print_report_border(FILE* out, const int* widths) {
    fputc('+', out);

    for (int i = 0; i < REPORT_COLUMN_COUNT; i++) {
        for (int j = 0; j < widths[i] + 2; j++) {
            fputc('-', out);
        }

        fputc('+', out);
    }

    fputc('\n', out);
}

static void print_report_row(
    FILE* out,
    const char* const* cells,
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

int print_report_output(char* in_f, char* out_f, const Table* table) {
    FILE* out = fopen(out_f, "w");

    if (!out) {
        return FUNC_RETURN_RUNTIME_ERROR;
    }

    int* widths = calloc(REPORT_COLUMN_COUNT, sizeof(*widths));
    if (!widths) {
        fclose(out);
        return FUNC_RETURN_RUNTIME_ERROR;
    }

    int result = FUNC_RETURN_SUCCESS;

    for (int i = 0; i < REPORT_COLUMN_COUNT; i++) {
        widths[i] = (int)strlen(report_header(i));
    }

    for (int i = 0; i < table->column_count; i++) {
        char* index = format_alloc("%d", i);

        if (!index) {
            result = FUNC_RETURN_RUNTIME_ERROR;
            goto cleanup;
        }

        widths[REPORT_INDEX] = our_max(
            widths[REPORT_INDEX],
            (int)strlen(index)
        );

        free(index);

        for (int j = REPORT_NAME; j < REPORT_COLUMN_COUNT; j++) {
            char* cell = format_report_cell(&table->columns[i], j);

            if (!cell) {
                result = FUNC_RETURN_RUNTIME_ERROR;
                goto cleanup;
            }

            widths[j] = our_max(widths[j], (int)strlen(cell));
            free(cell);
        }
    }

    fprintf(out, "CSV ANALYSIS REPORT\n\n");
    fprintf(out, "File: %s\n", in_f);
    fprintf(out, "Rows: %d\n", table->row_count);
    fprintf(out, "Columns: %d\n\n", table->column_count);

    fprintf(out, "Column summary:\n\n");

    print_report_border(out, widths);

    char** cells = calloc(REPORT_COLUMN_COUNT, sizeof(*cells));
    if (!cells) {
        result = FUNC_RETURN_RUNTIME_ERROR;
        goto cleanup;
    }

    for (int i = 0; i < REPORT_COLUMN_COUNT; i++) {
        cells[i] = (char*)report_header(i);
    }

    print_report_row(out, (const char* const*)cells, widths);

    for (int i = 0; i < REPORT_COLUMN_COUNT; i++) {
        cells[i] = NULL;
    }

    print_report_border(out, widths);

    for (int i = 0; i < table->column_count; i++) {
        cells[REPORT_INDEX] = format_alloc("%d", i);

        if (!cells[REPORT_INDEX]) {
            result = FUNC_RETURN_RUNTIME_ERROR;
            break;
        }

        for (int j = REPORT_NAME; j < REPORT_COLUMN_COUNT; j++) {
            cells[j] = format_report_cell(&table->columns[i], j);

            if (!cells[j]) {
                result = FUNC_RETURN_RUNTIME_ERROR;
                break;
            }
        }

        if (result != FUNC_RETURN_SUCCESS) {
            for (int j = 0; j < REPORT_COLUMN_COUNT; j++) {
                free(cells[j]);
                cells[j] = NULL;
            }
            break;
        }

        print_report_row(out, (const char* const*)cells, widths);

        for (int j = 0; j < REPORT_COLUMN_COUNT; j++) {
            free(cells[j]);
            cells[j] = NULL;
        }
    }

    free(cells);

    if (result != FUNC_RETURN_SUCCESS) {
        goto cleanup;
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

cleanup:
    free(widths);

    if (ferror(out)) {
        result = FUNC_RETURN_RUNTIME_ERROR;
    }

    if (fclose(out) != 0) {
        result = FUNC_RETURN_RUNTIME_ERROR;
    }

    return result;
}
