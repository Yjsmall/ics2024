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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
    int                NO;
    struct watchpoint *next;

    char   str[64];
    word_t old_val;

} WP;

static WP  wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
    int i;
    for (i = 0; i < NR_WP; i++) {
        wp_pool[i].NO = i;
        wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    }

    head = NULL;
    free_ = wp_pool;
}

void add_wp(char *str) {
    Assert(free_ != NULL, "sorry, no free memory for new watchpoint");

    bool    success = false;
    sword_t value = expr(str, &success);
    if (!success) {
        printf("error: wrong expression %s\n", str);
        return;
    }

    WP *ptr = free_;
    free_ = free_->next;
    int str_length = strlen(str);
    strncpy(ptr->str, str, str_length);
    ptr->str[str_length] = '\0';
    ptr->old_val = value;

    ptr->next = head;
    head = ptr;

    printf("Watchpoint %d: %s\n", ptr->NO, ptr->str);
}

void del_wp(int no) {
    WP dummy;
    dummy.next = head;
    WP *ptr = &dummy;
    while (ptr->next) {
        if (ptr->next->NO == no) {
            WP *tmp = ptr->next;
            ptr->next = tmp->next;
            tmp->next = free_;
            free_ = tmp;
            break;
        }
        ptr = ptr->next;
    }
    head = dummy.next;
}

int update_wp() {
    int n_changed = 0;
    WP *ptr = head;
    while (ptr != NULL) {
        bool   success = false;
        word_t value = expr(ptr->str, &success);
        Assert(success, "wrong expression %s\n", ptr->str);

        if (value != ptr->old_val) {
            n_changed += 1;
            printf("Watchpoint %d: %s\n", ptr->NO, ptr->str);
            printf("				Old value = 0x%08x(%d)\n", ptr->old_val, ptr->old_val);
            printf("				New value = 0x%08x(%d)\n", value, value);
            ptr->old_val = value;
        }
        ptr = ptr->next;
    }
    return n_changed;
}

void print_wp() {
    if (head == NULL) {
        printf("No watchpoints.\n");
        return;
    }

    printf("Num\t\tWhat\n");
    WP *ptr = head;
    while (ptr) {
        printf("%d\t\t%s\n", ptr->NO, ptr->str);
        ptr = ptr->next;
    }
}
