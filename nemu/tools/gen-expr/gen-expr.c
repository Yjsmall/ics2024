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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

// this should be enough
static char  buf[65536] = {};
static char  code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
    "#include <stdio.h>\n"
    "int main() { "
    "  unsigned result = %s; "
    "  printf(\"%%u\", result); "
    "  return 0; "
    "}";

// 用于控制表达式的最大深度，防止栈溢出和生成过长表达式
static int max_depth = 10;
// 用于跟踪当前表达式的长度，防止缓冲区溢出
static int buf_len = 0;

// 在字符串末尾添加字符
static void gen(char c) {
    buf[buf_len++] = c;
    buf[buf_len] = '\0';
}

// 在字符串末尾添加字符串
static void gen_str(const char *s) {
    int len = strlen(s);
    strcpy(buf + buf_len, s);
    buf_len += len;
}

// 随机选择 [0, n) 中的一个整数
static int choose(int n) {
    return rand() % n;
}

// 随机生成一个数字 (0-99)
static void gen_num() {
    int  num = choose(100);
    char num_str[10];
    sprintf(num_str, "%d", num);
    gen_str(num_str);
}

// 随机生成一个运算符
static void gen_rand_op() {
    char ops[] = {'+', '-', '*', '/'};
    // 随机选择一个运算符
    char op = ops[choose(4)];

    // 随机决定是否在运算符前后添加空格
    if (choose(2))
        gen(' ');
    gen(op);
    if (choose(2))
        gen(' ');
}

// 递归生成随机表达式
static void gen_rand_expr_internal(int depth) {
    // 如果深度过大或表达式长度接近缓冲区大小，则生成数字以结束表达式
    if (depth > max_depth || buf_len > 65000) {
        gen_num();
        return;
    }

    // 根据深度调整生成数字的概率，深度越大，生成数字的概率越高
    int choice;
    if (depth > max_depth / 2) {
        choice = choose(5); // 0: 数字, 1: 括号表达式, 2-4: 二元运算
    } else {
        choice = choose(3); // 0: 数字, 1: 括号表达式, 2: 二元运算
    }

    switch (choice) {
        case 0:
            // 生成一个数字
            gen_num();
            break;
        case 1:
            // 生成一个括号表达式
            gen('(');
            if (choose(2))
                gen(' '); // 随机在左括号后添加空格
            gen_rand_expr_internal(depth + 1);
            if (choose(2))
                gen(' '); // 随机在右括号前添加空格
            gen(')');
            break;
        default:
            // 生成一个二元运算表达式
            gen_rand_expr_internal(depth + 1);
            gen_rand_op();
            gen_rand_expr_internal(depth + 1);
            break;
    }
}

// 检查表达式是否有除以零的情况
static int check_division_by_zero() {
    // 编译并运行表达式，检查是否有运行时错误
    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    if (fp == NULL)
        return 1;
    fputs(code_buf, fp);
    fclose(fp);

    // 使用 -w 忽略警告
    return system("gcc -w /tmp/.code.c -o /tmp/.expr && /tmp/.expr > /dev/null 2>&1");
}

static void gen_rand_expr() {
    buf_len = 0;
    buf[0] = '\0';

    // 生成表达式，直到找到一个没有除以零的表达式
    do {
        buf_len = 0;
        buf[0] = '\0';
        gen_rand_expr_internal(0);
    } while (check_division_by_zero() != 0);
}

int main(int argc, char *argv[]) {
    int seed = time(0);
    srand(seed);
    int loop = 1;
    if (argc > 1) {
        sscanf(argv[1], "%d", &loop);
    }
    int i;
    for (i = 0; i < loop; i++) {
        gen_rand_expr();

        sprintf(code_buf, code_format, buf);

        FILE *fp = fopen("/tmp/.code.c", "w");
        assert(fp != NULL);
        fputs(code_buf, fp);
        fclose(fp);

        int ret = system("gcc -Werror /tmp/.code.c -o /tmp/.expr");
        if (ret != 0)
            continue;

        fp = popen("/tmp/.expr", "r");
        assert(fp != NULL);

        int result;
        ret = fscanf(fp, "%d", &result);
        pclose(fp);

        printf("%u %s\n", result, buf);
    }
    return 0;
}
