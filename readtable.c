#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "utils.h"
#include "readtable.h"

int read_one_line(FILE* input, char** buffer, int* cap){
    int len = 0;

    while(1){
        if (len + LEN_BLOCK > *cap){
            char* temp = realloc(*buffer, (*cap) * 2);
            if (!temp){
                return FUNC_RETURN_RUNTIME_ERROR;
            }
            else {
                *buffer = temp;
                (*cap) *= 2;
            }
        }
        if (fgets(*buffer + len, LEN_BLOCK, input) == NULL){
            if (ferror(input)) {
                return FUNC_RETURN_RUNTIME_ERROR;
            }
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
        return STATE_NULL;
    }

    char c;
    State result = STATE_NULL;

    char* buf_ptr = buf;

    while (1){
        c = *buf_ptr;
        buf_ptr++;

        if (c == '\0' || c == '\n'){
            break;
        }

        switch (result){
            case STATE_NULL:
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
                if (isdigit((unsigned char) c)) {
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

    if (result == STATE_INT && !is_valid_long_long(buf)){
        result = STATE_STRING;
    }
    else if (result == STATE_DOT){
        result = STATE_STRING;
    }
    else if (result == STATE_DOUBLE && !is_valid_double(buf)){
        result = STATE_STRING;
    }

    return result;
}

int parse_one_field(const char** src, char** buf, int* cap, State* initial_state){
    char c;
    int outer_quote_flag = 0;

    int cur_len = 0;

    while (**src == ' ' || **src == '\t'){
        (*src)++;
    }
    
    if (**src == '"'){
        outer_quote_flag = 1;
        (*src)++;
    }

    while (1){
        c = **(src);

        if (cur_len + 1 >= *cap) {
            char* temp = realloc(*buf, (*cap) * 2);
            if (!temp){
                return END_BY_ERROR_OVERFLOW;
            }
            else {
                (*buf) = temp;
                (*cap) *= 2;
            }
        }

        if (outer_quote_flag){
            switch (c){
                case '\0': case '\n':
                    (*buf)[cur_len] = '\0';
                    return END_BY_ERROR_QUOTE;
                
                case '"':
                    char next_c = *(*src + 1);

                    if (next_c == '"'){
                        (*buf)[cur_len++] = '"';
                        (*src)+=2;
                        break;
                    }

                    const char* forward_ptr = *src + 1;
                    while (*forward_ptr == ' ' || *forward_ptr == '\t') {
                        forward_ptr++;
                    }

                    next_c = *forward_ptr;
                    
                    switch (next_c){
                        case ',':
                            (*buf)[cur_len] = '\0';
                            *src = forward_ptr + 1;

                            if (initial_state){
                                if (cur_len == 0) {
                                    *initial_state = STATE_STRING;
                                }
                                else {
                                    *initial_state = STATE_NULL;
                                }
                            }
                            
                            return END_BY_COMMA;
                        
                        case '\0': case '\n':
                            (*buf)[cur_len] = '\0';
                            *src = forward_ptr;

                            if (initial_state){
                                if (cur_len == 0) {
                                    *initial_state = STATE_STRING;
                                }
                                else {
                                    *initial_state = STATE_NULL;
                                }
                            }
                            
                            return END_BY_LINE;

                        default:
                            (*buf)[cur_len] = '\0';
                            return END_BY_ERROR_QUOTE;
                    }
                    break;

                default:
                    (*buf)[cur_len++] = c;
                    (*src)++;
                    break;
            }
        }
        else {
            switch (c){
                case ' ': case '\t':
                    const char* forward_ptr = *src + 1;
                    while (*forward_ptr == ' ' || *forward_ptr == '\t') {
                        forward_ptr++;
                    }

                    if (*forward_ptr != '\n' && *forward_ptr != '\0' && *forward_ptr != ','){
                        (*buf)[cur_len] = '\0';
                        return END_BY_ERROR_QUOTE;
                    }
                    else {
                        (*src) = forward_ptr;
                    }
                    break;

                case '\n': case '\0':
                    (*buf)[cur_len] = '\0';
                    if (c == '\n') {
                        (*src)++;
                    }
                    return END_BY_LINE;

                case ',':
                    (*buf)[cur_len] = '\0';
                    (*src)++;
                    return END_BY_COMMA;

                case '"':
                    (*buf)[cur_len] = '\0';
                    return END_BY_ERROR_QUOTE;

                default:
                    (*buf)[cur_len++] = c;
                    (*src)++;
                    break;
            }
        }
    }
}