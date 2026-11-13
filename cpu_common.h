#ifndef CPU_COMMON_H
#define CPU_COMMON_H

#include <stdint.h>
#include <stdio.h>

#define MEM_SIZE 8192

enum { AOK = 1, HLT, ADR, INS };
enum { RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
       R8, R9, R10, R11, R12, R13, R14, RNONE };
enum { I_HALT, I_NOP, I_RRMOVQ, I_IRMOVQ, I_RMMOVQ, I_MRMOVQ,
       I_ALU, I_JMP, I_CALL, I_RET, I_PUSHQ, I_POPQ };

typedef struct {
    uint8_t memory[MEM_SIZE];
    uint64_t reg[16];
    uint64_t pc, valC, valP, valA, valB, valE, valM;
    int stat, zf, sf, of;
    int icode, ifun, rA, rB, cnd;
} CPU;

uint64_t read_mem(CPU *cpu, uint64_t addr, unsigned size);
void write_mem(CPU *cpu, uint64_t addr, unsigned size, uint64_t value);
int check_cond(const CPU *cpu, int ifun);
void fetch(CPU *cpu);
void decode(CPU *cpu);
void execute(CPU *cpu);
void memory_stage(CPU *cpu);
void write_back(CPU *cpu);
void update_pc(CPU *cpu);

#endif
