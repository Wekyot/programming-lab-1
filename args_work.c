#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "args_work.h"

void parse_args(int argc, char* argv[]){
    if (argc < 2){
        fprintf(stderr, "Вы запустили команду без аргументов. Попробуйте csvstat --help\n");
        return;
    }


    if (!strncmp(argv[1],"--help",7)){
        if (argc == 2)
            printf("Вы запросили справку программы csvstat.\nschema <csv_file_name> [options] выводит статистику по таблице: число строк данных, число столбцов, имя и тип каждого столбца в изначальном порядке.\nФормат вывода:\nrows <number>\ncolumns <number>\n<index> ""<name>"" <type>\n<index> ""<name>"" <type>\n...\n\nСпасибо, что пользуетесь нашей программой!\n\n");
        else {
            fprintf(stderr,"Лишний аргумент(ы) при использовании команды --help\n");
        }
    }
    else if (!strncmp(argv[1],"schema",7)){
        if (argc == 3){
            schema(argv[2]);
        }
        else {
            fprintf(stderr,"Недостаточно аргументов при использовании команды schema\nОжидалось: csvstat schema <название файла>\n");
        }
    }
    else if (!strncmp(argv[1],"stats",6)){
        if (argc >= 3){

        }
        else {
            fprintf(stderr,"Недостаточно аргументов при использовании команды stats\nОжидалось: csvstat stats <название файла>\n");
        }

    }
    else if (!strncmp(argv[1],"report",7)){
        if (argc >= 4){

        }
        else {
            fprintf(stderr,"Недостаточно аргументов при использовании команды report\n");
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
            return;
        }


        fprintf(stderr, "Некорректный синтаксис. Возможно вы имели в виду %s?\n",pretty_similar_command);
    }
}

void schema(char* f_name){
    FILE* input = open_file(f_name,"r");
    int cap = BASE_ARR_SIZE;
    char* buffer = (char*) create_arr(cap);
    Table table;
    table.column_count = 0;
    table.row_count = 0;
    table.columns = (Table_Column*) malloc(sizeof(Table_Column) * 256);
    
    read_one_line(input, &buffer, &cap);
    char* field_buf = (char*) create_arr(cap);

    const char* parser_ptr = buffer;
    while(1){
        int reason = parse_one_field(&parser_ptr, field_buf, cap, 1);

        table.columns[table.column_count].name = strdup(field_buf);
        table.columns[table.column_count].column_type = STATE_NONE;
        table.column_count++;

        if (reason == END_BY_LINE){
            break;
        }
    }

    if (table.column_count < 256){
        Table_Column* temp = realloc(table.columns, sizeof(Table_Column) * table.column_count);
        if (temp){
            table.columns = temp;
        }
        else {
            abort();
        }
    }

    while (read_one_line(input, &buffer, &cap)){
        table.row_count++;

        parser_ptr = buffer;
        int column_id = 0;

        while(1){
            int reason = parse_one_field(&parser_ptr, field_buf, cap, 0);

            if (column_id >= table.column_count){
                fprintf(stderr, "Ошибка в синтаксисе таблицы файла %s: в строке %d лишние столбцы.\n",f_name,column_id + 1);
                abort();
            }

            State cur_field = field_state(field_buf);

            update_column_schema(&(table.columns[column_id].column_type), cur_field);
            
            column_id++;
            if (reason == END_BY_LINE){
                break;
            }
        }
    }

    printf("rows %d\ncolumns %d\n",table.row_count, table.column_count);


    for (int i = 0;i < table.column_count;i++){
        printf("%d \"%s\" ",i,table.columns[i].name);
        fprintf_state(stdout, table.columns[i].column_type);
        printf("\n");
    }

    for (int i = 0;i < table.column_count;i++){
        free_but_better(table.columns[i].name);
    }
    free_but_better(table.columns);

    free(buffer);
    close_file(input);
}

void update_column_schema(State* column_state, State type){
    if (type > *column_state){
        *column_state = type;
    }
}
