#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "utils.h"
#include "readtable.h"

int read_one_line(FILE* input, char** buffer, int* cap){
    int len = 0;

    while(1){
        check_cap((void**)buffer, cap, len + LEN_BLOCK);
        if (fgets(*buffer + len, LEN_BLOCK, input) == NULL){
            return (len > 0);
        }

        len += strlen(*buffer + len);
        if (len > 0 && (*buffer)[len - 1] == '\n') {
            break; 
        }
    }
    return 1;
}

State field_state(char* buf){
    if (buf[0] == '\0'){
        return STATE_EMPTY;
    }

    char c;
    State result = STATE_NONE;

    char* buf_ptr = buf;

    while (1){
        c = *buf_ptr;
        buf_ptr++;

        if (c == '\0' || c == '\n'){
            break;
        }

        switch (result){
            case STATE_NONE:
                if (c == '+' || c == '-') {
                    result = STATE_SIGN;
                } else if (isdigit(c)) {
                    result = STATE_INT;
                } else {
                    result = STATE_STRING;
                }
                break;

            case STATE_SIGN:
                if (isdigit(c)) {
                    result = STATE_INT;
                } else {
                    result = STATE_STRING;
                }
                break;

            case STATE_INT:
                if (isdigit(c)) {
                    result = STATE_INT;
                } else if (c == '.') {
                    result = STATE_DOT;
                } else {
                    result = STATE_STRING;
                }
                break;

            case STATE_DOT:
                if (isdigit(c)) {
                    result = STATE_DOUBLE;
                }
                else {
                    result = STATE_STRING;
                }
                break;

            case STATE_DOUBLE:
                if (isdigit(c)) {
                    result = STATE_DOUBLE;
                } else {
                    result = STATE_STRING;
                }
                break;

            case STATE_STRING:
                break;
        }
    }

    if (result == STATE_INT && (strlen(buf) > 19 || (strlen(buf) == 19 && strcmp(buf, "9223372036854775807") > 0))){
        result = STATE_STRING;
    }
    else if (result == STATE_DOT){
        result = STATE_STRING;
    }

    return result;
}

int parse_one_field(const char** src, char* buf, size_t max_len, int is_header){
    char c;
    int exit_flag = 0;
    int outer_quote_flag = 0;
    int was_quote_flag = 0;

    int cur_len = 0;

    int ret_reason;

    while (!exit_flag){
        c = (**src);
        (*src)++;

        switch (c){
            case '\n': case '\0':
                buf[cur_len] = '\0';
                ret_reason = END_BY_LINE;
                exit_flag = 1;
                break;

            case ',':
                if (was_quote_flag || !outer_quote_flag){
                    outer_quote_flag = 0;
                    was_quote_flag = 0;

                    buf[cur_len] = '\0';
                    ret_reason = END_BY_COMMA;
                    exit_flag = 1;
                }
                else{
                    buf[cur_len] = c;
                    cur_len++;
                }
                break;

            case '"':
                if (!outer_quote_flag){
                    outer_quote_flag = 1;
                    was_quote_flag = 0;
                }
                else{
                    if (!was_quote_flag){
                        was_quote_flag = 1;
                    }
                    else{
                        buf[cur_len] = c;
                        cur_len++;
                        was_quote_flag = 0;
                    }
                }
                break;

            default:
                buf[cur_len] = c;
                cur_len++;
                break;
        }
    }

    return ret_reason;
}