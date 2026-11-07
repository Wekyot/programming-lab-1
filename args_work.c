#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "args_work.h"

void parse_args(int argc, char* argv[]){
    if (argc < 2){
        fprintf(stderr, "Вы запустили команду без аргументов. Попробуйте csvstat --help\n");
        exit(2);
    }

    if (!strncmp(argv[1],"--help",7)){
        if (argc == 2)
            printf("Вы запросили справку программы csvstat.\nschema <csv_file_name> [options] выводит статистику по таблице: число строк данных, число столбцов, имя и тип каждого столбца в изначальном порядке.\nФормат вывода:\nrows <number>\ncolumns <number>\n<index> ""<name>"" <type>\n<index> ""<name>"" <type>\n...\n\nСпасибо, что пользуетесь нашей программой!\n\n");
        else {
            fprintf(stderr,"Лишний аргумент(ы) при использовании команды --help\n");
            exit(2);
        }
    }
    else if (!strncmp(argv[1],"schema",7)){
        if (argc == 3){
            schema(argv[2]);
        }
        else {
            fprintf(stderr,"Недостаточно аргументов при использовании команды schema\nОжидалось: csvstat schema <название файла>\n");
            exit(2);
        }
    }
    else if (!strncmp(argv[1],"stats",6)){
        if (argc >= 3){
            stats(argv[2]);
        }
        else {
            fprintf(stderr,"Недостаточно аргументов при использовании команды stats\nОжидалось: csvstat stats <название файла>\n");
            exit(2);
        }

    }
    else if (!strncmp(argv[1],"report",7)){
        if (argc >= 4){
            report(argv[2],argv[3]);
        }
        else {
            fprintf(stderr,"Недостаточно аргументов при использовании команды report\n");
            exit(2);
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
            exit(2);
        }

        fprintf(stderr, "Некорректный синтаксис. Возможно вы имели в виду %s?\n",pretty_similar_command);
        exit(2);
    }
}

void schema(char* f_name){
    Table table = { .column_count = 0, .row_count = 0, .columns = my_malloc(sizeof(Table_Column) * 256) };

    parse_csv_stream(f_name, &table, handle_cell_for_schema);

    print_schema_output(&table);
    
    free_table_resources(&table);
}

void stats(char* f_name){
    Table table = { .column_count = 0, .row_count = 0, .columns = my_malloc(sizeof(Table_Column) * 256) };

    parse_csv_stream(f_name, &table, handle_cell_for_stats);

    print_stats_output(&table);
    
    free_table_resources(&table);
}

void report(char* f_name, char* out_f_name){
    Table table = { .column_count = 0, .row_count = 0, .columns = my_malloc(sizeof(Table_Column) * 256) };

    parse_csv_stream(f_name, &table, handle_cell_for_stats);

    print_report_output(f_name, out_f_name, &table);
    
    free_table_resources(&table);
}

void parse_csv_stream(const char* f_name, Table* table, CellHandler handle_cell){
    FILE* input = open_file(f_name,"r");
    int cap = BASE_ARR_SIZE;
    char* buffer = (char*) create_arr(cap);
    
    read_one_line(input, &buffer, &cap);
    char* field_buf = (char*) create_arr(cap);

    const char* parser_ptr = buffer;
    while(1){
        int reason = parse_one_field(&parser_ptr, field_buf, cap, NULL);

        if (strlen(field_buf) == 0){
            fprintf(stderr, "Пустое имя столбца %d\n",table->column_count + 1);
            exit(1);
        }

        for (int i = 0; i < table->column_count; i++) {
            if (strcmp(table->columns[i].name, field_buf) == 0) {
                // Если нашли совпадение — это дубликат, бросаем ошибку
                fprintf(stderr, "Ошибка: Дублирование имени столбца \"%s\" (столбцы %d и %d)\n", field_buf, i + 1, table->column_count + 1);
                abort();
            }
        }

        init_column_stats(&(table->columns[table->column_count]), field_buf);
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
            abort();
        }
    }

    while (read_one_line(input, &buffer, &cap)){
        table->row_count++;

        parser_ptr = buffer;
        int column_id = 0;

        while(1){
            State cur_field_state = STATE_NULL;
            int reason = parse_one_field(&parser_ptr, field_buf, cap, &cur_field_state);

            if (reason == END_BY_ERROR_OVERFLOW){
                fprintf(stderr, "Переполнение буфера при парсинге поля \"%s\" в строке %d столбца %d\n", field_buf, table->row_count, column_id + 1);
                exit(1);
            }
            else if (reason == END_BY_ERROR_QUOTE){
                fprintf(stderr, "Ошибка в расстановке кавычек при парсинге поля \"%s\" в строке %d столбца %d\n", field_buf, table->row_count, column_id + 1);
                exit(1);
            }

            if (column_id >= table->column_count){
                fprintf(stderr, "Ошибка в синтаксисе таблицы файла %s: в строке %d лишние столбцы.\n",f_name,column_id + 1);
                abort();
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
                fprintf(stderr, "Ошибка в синтаксисе таблицы файла %s: в строке %d недостаточно столбцов.\n",f_name,column_id + 1);
                abort();
            }
    }

    free(buffer);
    free(field_buf);
    close_file(input);
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
    switch ((*col).column_type){
        case STATE_INT:
            long long int num = strtoll(field_buf, &end_ptr, 10);
            if (num % 2 == 0){
                (*col).INT.even++;
            }

        case STATE_DOUBLE:
            double val = strtod(field_buf, &end_ptr);
            if (val > (*col).DOUBLE.max){
                (*col).DOUBLE.max = val;
            }
            if (val < (*col).DOUBLE.min){
                (*col).DOUBLE.min = val;
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
    col->INT.even = 0;

    col->DOUBLE.integer = 0;
    col->DOUBLE.max = -DBL_MAX;
    col->DOUBLE.min = DBL_MAX;
    col->DOUBLE.mean = 0.0;
    col->DOUBLE.positive = 0;
    col->DOUBLE.negative = 0;

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
    free(table->columns);
}

void print_schema_output(const Table* table){
    printf("rows %d\ncolumns %d\n", table->row_count, table->column_count);
    for (int i = 0; i < table->column_count;i++){
        printf("%d ",i);
        print_column_name(stdout, table->columns[i].name);
        fprintf_state(stdout, table->columns[i].column_type);
        printf("\n");
    }
}

void print_stats_output(const Table* table){
    printf("rows %d\ncolumns %d\n\n", table->row_count, table->column_count);

    for (int i = 0;i < table->column_count;i++){
        printf("column %d\nname ",i);
        print_column_name(stdout, table->columns[i].name);
        printf("\ntype ");
        fprintf_state(stdout, table->columns[i].column_type);
        printf("\nnulls %d\n",table->columns[i].STRING.nulls);
        switch (table->columns[i].column_type){
            case STATE_NULL:
                printf("rows %d\n",table->row_count);
                break;
            
            case STATE_INT:
                printf("min %d\nmax %d\nmean %.6lf\npositive %d\nnegative %d\neven %d\n",(int) table->columns[i].DOUBLE.min, (int) table->columns[i].DOUBLE.max, table->columns[i].DOUBLE.mean, table->columns[i].DOUBLE.positive, table->columns[i].DOUBLE.negative, table->columns[i].INT.even);
                break;

            case STATE_DOUBLE:
                printf("min %.6lf\nmax %.6lf\nmean %.6lf\npositive %d\nnegative %d\ninteger %d\n",table->columns[i].DOUBLE.min, table->columns[i].DOUBLE.max, table->columns[i].DOUBLE.mean, table->columns[i].DOUBLE.positive, table->columns[i].DOUBLE.negative, table->columns[i].DOUBLE.integer);
                break;
            
            case STATE_STRING:
                printf("min_length %d\nmax_length %d\nmean_length %.6lf\nempty %d\n",table->columns[i].STRING.min_length, table->columns[i].STRING.max_length, table->columns[i].STRING.mean_length, table->columns[i].STRING.empty);
                break;
        }
        printf("\n");
    }
}

void print_report_output(char* in_f, char* out_f, const Table* table){
    FILE* out = open_file(out_f, "w");

    int column_widths[10] = {3, 13, 8, 8, 10, 9, 10, 9, 15, 15};
    
    column_widths[0] = __max(column_widths[0],len_of_int(table->column_count));


    for (int i = 0;i < table->column_count;i++){
        column_widths[1] = __max(column_widths[1], strlen(table->columns[i].name));

        column_widths[3] = __max(column_widths[3], table->columns[i].STRING.nulls);

        column_widths[4] = __max(column_widths[4], table->columns[i].STRING.valid_count);

        column_widths[5] = __max(column_widths[5], len_of_int(( (int) table->columns[i].DOUBLE.min)) + 2);

        column_widths[6] = __max(column_widths[6], len_of_int(( (int) table->columns[i].DOUBLE.max)) + 2);

        column_widths[7] = __max(column_widths[7], len_of_int(( (int) table->columns[i].DOUBLE.mean)) + 2);

        column_widths[8] = __max(column_widths[8], len_of_int(table->columns[i].DOUBLE.positive));

        column_widths[9] = __max(column_widths[9], len_of_int(table->columns[i].DOUBLE.negative));
    }

    int total_width = 11;
    for (int i = 0;i < 10;i++){
        total_width += column_widths[i];
    }

    print_char_line(out, '=', total_width, 1);
    print_char_line(out, ' ', total_width / 3, 0);
    fprintf(out, "ОТЧЕТ ПО АНАЛИЗУ CSV-ТАБЛИЦЫ\n");
    print_char_line(out, '=', total_width, 1);
    fprintf(out, "Файл: %s\nВсего строк данных: %d\nВсего столбцов: %d\n", in_f, table->row_count, table->column_count);
    print_char_line(out, '-', total_width, 1);

    fprintf(out, "\nСводная статистика по столбцам:\n\n");
    print_table(out, column_widths);
    fprintf(out, "| № | Имя столбца | Тип    | Пустые | Валидные | Минимум | Максимум | Среднее | Положительное | Отрицательное |\n");

    for (int i = 0; i < table->column_count; i++){
        print_table(out, column_widths);
        fprintf(out, "| %d | %-*s | %-*s", i, column_widths[1] - 2, table->columns[i].name, column_widths[2] - 1, state_to_str(table->columns[i].column_type));
        fprintf(out, "| %*d | %*d | %*.6lf | %*.6lf | %*.6lf", column_widths[3] - 2, table->columns[i].STRING.nulls, column_widths[4] - 2, table->columns[i].STRING.valid_count, column_widths[5] - 2, table->columns[i].column_type == STATE_STRING ? table->columns[i].STRING.min_length : table->columns[i].DOUBLE.min, column_widths[6] - 2, table->columns[i].column_type == STATE_STRING ? table->columns[i].STRING.max_length : table->columns[i].DOUBLE.max,  column_widths[7] - 2, table->columns[i].column_type == STATE_STRING ? table->columns[i].STRING.mean_length : table->columns[i].DOUBLE.mean,  column_widths[8] - 2);
        if (table->columns[i].column_type == STATE_INT || table->columns[i].column_type == STATE_DOUBLE){
            fprintf(out, " | %*d | %*d |\n", column_widths[8] - 2, table->columns[i].DOUBLE.positive, column_widths[9] - 2, table->columns[i].DOUBLE.negative);
        }
        else {
            fprintf(out, " | %*s | %*s |\n", column_widths[8] - 2, "N/A", column_widths[9] - 2, "N/A");
        }
    }
    print_table(out, column_widths);
    fprintf(out, "\n\nСпецифические метрики:\n");

    for (int i = 0;i < table->column_count; i++){
        fprintf(out, " - Столбец %d ",i);
        print_column_name(out, table->columns[i].name);

        switch (table->columns[i].column_type)
        {
        case STATE_NULL:
            fprintf(out, "Количество пустых полей в столбце: %d\n", table->row_count);
            break;
        case STATE_INT:
            fprintf(out, "Количество четных чисел в столбце: %d\n", table->columns[i].INT.even);
            break;
        case STATE_DOUBLE:
            fprintf(out, "Количество целых чисел в столбце: %d\n", table->columns[i].DOUBLE.integer);
            break;
        case STATE_STRING:
            fprintf(out, "Количество пустых строк в столбце: %d\n", table->columns[i].STRING.empty);
            break;
        }
    }

    print_char_line(out, '-', total_width, 1);

    close_file(out);
}