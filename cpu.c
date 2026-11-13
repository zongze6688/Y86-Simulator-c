#include "cpu_common.h"

/* 每个阶段通过 CPU 结构体中的暂存值传递数据。请补全以下六个阶段。 */
void fetch(CPU *cpu)
{
    uint64_t byte0 = read_mem(cpu, cpu->pc, 1);
    if (cpu->stat != AOK) return;
    cpu->icode = (int)(byte0 >> 4);
    cpu->ifun = (int)(byte0 & 15);

    /* TODO: 校验指令和寄存器编码，处理 halt，读取寄存器字节和立即数，计算 valP。
       完成实现后删除下面的占位状态。 */
    cpu->stat = INS;
}

void decode(CPU *cpu)
{
    /* TODO: 根据指令选择源寄存器，将读取的值写入 valA 和 valB。
       栈操作隐式使用 RSP，RNONE 对应的值为 0。 */
    (void)cpu;
}

void execute(CPU *cpu)
{
    /* TODO: 选择 ALU 操作数并计算 valE。OPq 更新 ZF、SF、OF。
       条件传送和跳转使用 check_cond 计算 cnd。使用无符号运算实现 64 位回绕。 */
    cpu->valE = 0;
    cpu->cnd = 0;
}

void memory_stage(CPU *cpu)
{
    /* TODO: 根据指令访问内存。pushq 写入 valA，call 写入 valP。
       读取操作将结果写入 valM，使用 read_mem 和 write_mem 检查地址。 */
    cpu->valM = 0;
}

void write_back(CPU *cpu)
{
    /* TODO: 选择 dstE、dstM 并写入寄存器。
       若两者均为 RSP，valM 应覆盖 valE。 */
    (void)cpu;
}

void update_pc(CPU *cpu)
{
    /* TODO: 按 call、条件跳转、ret 或顺序执行选择下一条指令的地址。 */
    (void)cpu;
}
