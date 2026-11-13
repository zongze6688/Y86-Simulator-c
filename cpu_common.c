#include "cpu_common.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

uint64_t read_mem(CPU *cpu, uint64_t addr, unsigned size)
{
    uint64_t value = 0;
    if (size > MEM_SIZE || addr > MEM_SIZE - size) {
        cpu->stat = ADR;
        return 0;
    }
    for (unsigned i = 0; i < size; i++)
        value |= (uint64_t)cpu->memory[addr + i] << (8 * i);
    return value;
}

void write_mem(CPU *cpu, uint64_t addr, unsigned size, uint64_t value)
{
    if (size > MEM_SIZE || addr > MEM_SIZE - size) {
        cpu->stat = ADR;
        return;
    }
    for (unsigned i = 0; i < size; i++)
        cpu->memory[addr + i] = (uint8_t)(value >> (8 * i));
}

int check_cond(const CPU *cpu, int ifun)
{
    switch (ifun) {
    case 0: return 1;
    case 1: return (cpu->sf ^ cpu->of) || cpu->zf;
    case 2: return cpu->sf ^ cpu->of;
    case 3: return cpu->zf;
    case 4: return !cpu->zf;
    case 5: return !(cpu->sf ^ cpu->of);
    case 6: return !(cpu->sf ^ cpu->of) && !cpu->zf;
    default: return 0;
    }
}

static int hex_digit(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/*
 * Portable replacement for POSIX getline(): reads one line from `input` into
 * `*line`, keeping the terminating '\n' when present, and NUL-terminates it.
 * The buffer grows on demand, so lines of any length are supported.
 * Returns 1 when a line was read, 0 at end of file, and -1 when the buffer
 * could not be grown (out of memory).
 */
static int read_line(char **line, size_t *capacity, FILE *input)
{
    size_t length = 0;
    int c;
    while ((c = fgetc(input)) != EOF) {
        if (length + 2 > *capacity) {
            size_t new_capacity = *capacity ? *capacity * 2 : 128;
            char *grown = realloc(*line, new_capacity);
            if (!grown) return -1;
            *line = grown;
            *capacity = new_capacity;
        }
        (*line)[length++] = (char)c;
        if (c == '\n') break;
    }
    if (length == 0 && c == EOF) return 0;
    (*line)[length] = '\0';
    return 1;
}

static int load_program(CPU *cpu, FILE *input)
{
    char *line = NULL;
    size_t capacity = 0, line_number = 0;
    int result = 0, status = 0;
    while ((status = read_line(&line, &capacity, input)) > 0) {
        line_number++;
        char *comment = strchr(line, '|');
        if (comment) *comment = '\0';
        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';
        char *address = line;
        while (isspace((unsigned char)*address)) address++;
        char *data = colon + 1;
        while (isspace((unsigned char)*data)) data++;
        if (!*address || !*data) continue;
        char *end;
        errno = 0;
        if (*address == '-') goto invalid;
        uintmax_t addr = strtoumax(address, &end, 16);
        while (isspace((unsigned char)*end)) end++;
        if (errno || end == address || *end) goto invalid;
        while (*data) {
            while (isspace((unsigned char)*data)) data++;
            if (!*data) break;
            int high = hex_digit((unsigned char)data[0]);
            int low = data[1] ? hex_digit((unsigned char)data[1]) : -1;
            if (high < 0 || low < 0) goto invalid;
            if (addr < MEM_SIZE) cpu->memory[addr] = (uint8_t)((high << 4) | low);
            if (addr < UINTMAX_MAX) addr++;
            data += 2;
        }
        continue;
invalid:
        fprintf(stderr, "Invalid .yo input at line %zu\n", line_number);
        result = -1;
        break;
    }
    if (status < 0) {
        fputs("Out of memory reading .yo input\n", stderr);
        result = -1;
    }
    if (ferror(input)) {
        perror("Reading .yo input");
        result = -1;
    }
    free(line);
    return result;
}

static int64_t signed_value(uint64_t value)
{
    if (value <= INT64_MAX) return (int64_t)value;
    return -1 - (int64_t)(UINT64_MAX - value);
}

static void dump_state(const CPU *cpu)
{
    static const char *names[] = {
        "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14"
    };
    printf("{\"CC\":{\"ZF\":%d,\"SF\":%d,\"OF\":%d},\"MEM\":{",
           cpu->zf, cpu->sf, cpu->of);
    int first = 1;
    for (unsigned addr = 0; addr < MEM_SIZE; addr += 8) {
        uint64_t value = 0;
        for (unsigned i = 0; i < 8; i++)
            value |= (uint64_t)cpu->memory[addr + i] << (8 * i);
        if (value) {
            printf("%s\"%u\":%" PRId64, first ? "" : ",", addr, signed_value(value));
            first = 0;
        }
    }
    printf("},\"PC\":%" PRIu64 ",\"REG\":{", cpu->pc);
    for (unsigned i = 0; i < 15; i++)
        printf("%s\"%s\":%" PRId64, i ? "," : "", names[i], signed_value(cpu->reg[i]));
    printf("},\"STAT\":%d}", cpu->stat);
}

int main(void)
{
    CPU cpu = {0};
    cpu.stat = AOK;
    cpu.zf = 1;
    cpu.icode = I_NOP;
    cpu.rA = cpu.rB = RNONE;
    if (load_program(&cpu, stdin) != 0) return 1;
    puts("[");
    int first = 1;
    while (cpu.stat == AOK) {
        fetch(&cpu);
        if (cpu.stat == AOK) {
            decode(&cpu);
            execute(&cpu);
            memory_stage(&cpu);
            write_back(&cpu);
            if (cpu.stat == AOK) update_pc(&cpu);
        }
        if (!first) puts(",");
        dump_state(&cpu);
        first = 0;
    }
    puts("\n]");
    return ferror(stdout) ? 1 : 0;
}
