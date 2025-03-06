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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <string.h>
#include "sdb.h"
#include "common.h"
#include "debug.h"
#include "memory/paddr.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets() {
    static char *line_read = NULL;

    if (line_read) {
        free(line_read);
        line_read = NULL;
    }

    line_read = readline("(nemu) ");

    if (line_read && *line_read) {
        add_history(line_read);
    }

    return line_read;
}

static int cmd_c(char *args) {
    cpu_exec(-1);
    return 0;
}

static int cmd_q(char *args) {
    nemu_state.state = NEMU_QUIT;
    return -1;
}

static int cmd_si(char *args) {
    if (args == NULL) {
        cpu_exec(1);
        return 0;
    }

    char *endptr;
    long  num = strtol(args, &endptr, 10);

    if (*endptr != '\0') {
        Log("The string is not a valid integer.\n");
    } else {
        cpu_exec(num);
    }

    return 0;
}

static int cmd_info(char *args) {
    Log("args is args: %s\n", args);
    if (strcmp(args, "r") == 0) {
        isa_reg_display();
    }
    return 0;
}

static int cmd_x(char *args) {
    Log("x args is args: %s\n", args);
    int size, addr;

    int result = sscanf(args, "%d 0x%x", &size, &addr);

    if (result == 2) {
        for (int i = 0; i < size; i += 4, addr += 4) {
            printf(FMT_PADDR ": ", addr + i);
            word_t   paddr = paddr_read(addr, 4);
            uint8_t *ptr = (uint8_t *)&paddr;

            for (int j = 0; j < 4; j++) {
                printf("%02x ", ptr[j]);
            }
            printf("\n");
        }
    } else {
        printf("Failed to parse the string.\n");
    }
    return 0;
}

static int cmd_p(char *args) {
    Log("p args is args: %s\n", args);
    bool   success;
    word_t result = expr(args, &success);
    if (success) {
        printf("%s = %d\n", args, result);
    } else {
        Log("oh no expr fail\n");
    }
    return 0;
}

static int cmd_help(char *args);

static struct {
    const char *name;
    const char *description;
    int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c",    "Continue the execution of the program",            cmd_c   },
    {"q",    "Exit NEMU",                                        cmd_q   },
    {"si",   "Execute instructions",                             cmd_si  },
    {"info", "Print program status",                             cmd_info},
    {"x",    "Scan memory",                                      cmd_x   },
    {"p",    "expression evaluation",                            cmd_p   },

    /* TODO: Add more commands */
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
    /* extract the first argument */
    char *arg = strtok(NULL, " ");
    int   i;

    if (arg == NULL) {
        /* no argument given */
        for (i = 0; i < NR_CMD; i++) {
            printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        }
    } else {
        for (i = 0; i < NR_CMD; i++) {
            if (strcmp(arg, cmd_table[i].name) == 0) {
                printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
                return 0;
            }
        }
        printf("Unknown command '%s'\n", arg);
    }
    return 0;
}

void sdb_set_batch_mode() {
    is_batch_mode = true;
}

void sdb_mainloop() {
    if (is_batch_mode) {
        cmd_c(NULL);
        return;
    }

    for (char *str; (str = rl_gets()) != NULL;) {
        char *str_end = str + strlen(str);

        /* extract the first token as the command */
        char *cmd = strtok(str, " ");
        if (cmd == NULL) {
            continue;
        }

        /* treat the remaining string as the arguments,
     * which may need further parsing
     */
        char *args = cmd + strlen(cmd) + 1;
        if (args >= str_end) {
            args = NULL;
        }

#ifdef CONFIG_DEVICE
        extern void sdl_clear_event_queue();
        sdl_clear_event_queue();
#endif

        int i;
        for (i = 0; i < NR_CMD; i++) {
            if (strcmp(cmd, cmd_table[i].name) == 0) {
                if (cmd_table[i].handler(args) < 0) {
                    return;
                }
                break;
            }
        }

        if (i == NR_CMD) {
            printf("Unknown command '%s'\n", cmd);
        }
    }
}

void init_sdb() {
    /* Compile the regular expressions. */
    init_regex();

    /* Initialize the watchpoint pool. */
    init_wp_pool();
}
