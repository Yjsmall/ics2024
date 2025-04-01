#include "sdb.h"

// Instruction Trace
#define IRINGBUF_SIZE 64
typedef struct {
    uint32_t pc[IRINGBUF_SIZE];
    char     disasm[IRINGBUF_SIZE][64];
    uint8_t  head;
    uint8_t  count;
} IRingBuf;

static IRingBuf iringbuf;
// Instruction Trace
void iringbuf_add(uint32_t pc, const char *disasm_str) {
    // 写入新的指令信息
    iringbuf.pc[iringbuf.head] = pc;
    strncpy(iringbuf.disasm[iringbuf.head], disasm_str, sizeof(iringbuf.disasm[0]) - 1);
    iringbuf.disasm[iringbuf.head][sizeof(iringbuf.disasm[0]) - 1] = '\0'; // 确保字符串以 NULL 结尾

    // 更新 head 和 count
    iringbuf.head = (iringbuf.head + 1) % IRINGBUF_SIZE; // 循环更新 head
    if (iringbuf.count < IRINGBUF_SIZE) {
        iringbuf.count++; // 如果缓冲区未满，增加计数
    }
}

void handle_error(vaddr_t error_pc) {
    printf("Error occurred at PC: 0x%08x\n", error_pc);

    // 标记出错的指令
    for (int i = 0; i < iringbuf.count; i++) {
        int idx = (iringbuf.head - iringbuf.count + i + IRINGBUF_SIZE) % IRINGBUF_SIZE;
        if (iringbuf.pc[idx] == error_pc) {
            printf("--> PC: 0x%08x | Disasm: %s\n", iringbuf.pc[idx], iringbuf.disasm[idx]);
        } else {
            printf("    PC: 0x%08x | Disasm: %s\n", iringbuf.pc[idx], iringbuf.disasm[idx]);
        }
    }
}

// Memory Trace
