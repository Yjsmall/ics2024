/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "common.h"
#include "debug.h"
#include <isa.h>
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

enum {
    TK_NOTYPE = 256,
    TK_EQ,
    TK_INTEGER,
};

static struct rule {
    const char *regex;
    int         token_type;
} rules[] = {
    {" +",     TK_NOTYPE },
    {"[0-9]+", TK_INTEGER},
    {"\\(",    '('       },
    {"\\)",    ')'       },
    {"\\+",    '+'       },
    {"\\-",    '-'       },
    {"\\*",    '*'       },
    {"\\/",    '/'       },
    {"==",     TK_EQ     },
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
    int  i;
    char error_msg[128];
    int  ret;

    for (i = 0; i < NR_REGEX; i++) {
        ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
        if (ret != 0) {
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
        }
    }
}

typedef struct token {
    int  type;
    char *str;
    int  str_capacity; 
} Token;

#define INIT_TOKEN_CAPACITY 32
#define INIT_STR_CAPACITY   32

static Token *tokens __attribute__((used)) = NULL;
static int    tokens_capacity = 0; 
static int   nr_token __attribute__((used)) = 0;

static void init_tokens() {
    if (tokens_capacity == 0) {
        tokens_capacity = INIT_TOKEN_CAPACITY;
        tokens = malloc(tokens_capacity * sizeof(Token));
        Assert(tokens != NULL, "Failed to allocate memory for tokens");
        for (int i = 0; i < tokens_capacity; i++) {
            tokens[i].str = malloc(INIT_STR_CAPACITY);
            tokens[i].str_capacity = INIT_STR_CAPACITY;
            tokens[i].str[0] = '\0'; 
        }
    }
}

static void reset_tokens() {
    if (tokens) {
        for (int i = 0; i < tokens_capacity; i++) {
            free(tokens[i].str);
        }
        free(tokens);
        tokens = NULL;
        tokens_capacity = 0;
    }
    nr_token = 0;
}

static bool expand_tokens_array() {
    int new_capacity = tokens_capacity * 2;
    Token *new_tokens = realloc(tokens, new_capacity * sizeof(Token));
    if (!new_tokens) return false;

    for (int i = tokens_capacity; i < new_capacity; i++) {
        new_tokens[i].str = malloc(INIT_STR_CAPACITY);
        new_tokens[i].str_capacity = INIT_STR_CAPACITY;
        new_tokens[i].str[0] = '\0';
    }

    tokens = new_tokens;
    tokens_capacity = new_capacity;
    return true;
}

static bool expand_str(Token *token, int required_len) {
    int new_capacity = token->str_capacity;
    while (new_capacity < required_len) {
        new_capacity *= 2;
    }

    char *new_str = realloc(token->str, new_capacity);
    if (!new_str) return false;

    token->str = new_str;
    token->str_capacity = new_capacity;
    return true;
}

static bool make_token(char *e) {
    init_tokens();
    int        position = 0;
    int        i;
    regmatch_t pmatch;

    nr_token = 0;

    while (e[position] != '\0') {
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                char *substr_start = e + position;
                int   substr_len = pmatch.rm_eo;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                    i, rules[i].regex, position, substr_len, substr_len, substr_start);

                position += substr_len;

                if (rules[i].token_type == TK_NOTYPE) {
                    break;
                }

                if (nr_token >= tokens_capacity) {
                    if (!expand_tokens_array()) {
                        reset_tokens();
                        Assert(0, "Failed to expand tokens array");
                    }
                }

                switch (rules[i].token_type) {
                    case TK_INTEGER:
                        if (substr_len > tokens[nr_token].str_capacity) {
                            if (!expand_str(&tokens[nr_token], substr_len + 1)) {
                                reset_tokens();
                                Assert(0, "Failed to expand string");
                            }
                        }
                        strncpy(tokens[nr_token].str, substr_start, substr_len);
                        tokens[nr_token].str[substr_len] = '\0';
                        tokens[nr_token].type = rules[i].token_type;
                        break;
                    case '+':
                    case '-':
                    case '*':
                    case '/':
                    case '(':
                    case ')':
                        tokens[nr_token].type = rules[i].token_type;
                        break;
                    default: TODO();
                }

                nr_token++;
                break;
            }
        }

        if (i == NR_REGEX) {
            printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
            return false;
        }
    }

    return true;
}

static bool check_parentheses(size_t r, size_t l) {
    int    parent_cnt = 0;
    bool   main_operator_mode = false;
    size_t p = r;
    while (p <= l) {
        if (tokens[p].type == '(') {
            parent_cnt++;
        }
        if (tokens[p].type == ')') {
            parent_cnt--;
        }
        if (parent_cnt < 0) {
            Assert(0, "illegel expression!!!");
        }

        if (parent_cnt == 0 && p < l) {
            main_operator_mode = true;
        }
        p++;
    }

    if (parent_cnt != 0) {
        Assert(0, "illegel expression!!!");
    }

    if (main_operator_mode) {
        return false;
    }

    return true;
}

size_t op_pos(size_t p, size_t q) {
    int par_cnt = 0;
    int op_idx = -1;
    while (p < q) {
        if (tokens[p].type == '(') {
            par_cnt++;
        } else if (tokens[p].type == ')') {
            par_cnt--;
        } else if (par_cnt == 0) {
            if (tokens[p].type == '+' || tokens[p].type == '-') {
                op_idx = p;
            } else if (tokens[p].type == '*' || tokens[p].type == '/') {
                op_idx = p;
            }
        }

        p++;
    }
    return op_idx;
}

word_t eval(size_t p, size_t q) {
    if (p > q) {
        Assert(0, "Invalid expressions");
    } else if (p == q) {
        return atoi(tokens[p].str);
    } else if (check_parentheses(p, q) == true) {
        return eval(p + 1, q - 1);
    } else {
        size_t op_idx = op_pos(p, q);
        word_t val1 = eval(p, op_idx - 1);
        word_t val2 = eval(op_idx + 1, q);
        switch (tokens[op_idx].type) {
            case '+': return val1 + val2;
            case '-': return val1 - val2;
            case '*': return val1 * val2;
            case '/':
                Assert(val2 != 0, "ZeroDivisionError: division by zero");
                return val1 / val2;
            default: TODO();
        }
    }

    return 0;
}

word_t expr(char *e, bool *success) {
    if (!make_token(e)) {
        *success = false;
        return 0;
    }

    *success = true;
    word_t result = eval(0, nr_token - 1);
    reset_tokens();
    return result;
}
