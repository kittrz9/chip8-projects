#ifndef CPU_H
#define CPU_H

#include <stdint.h>

extern uint8_t ram[0x1000];

typedef struct {
	uint8_t v[0x10];
	uint16_t i;
	uint16_t pc;
	uint16_t stack[16];
	uint8_t sp;
	uint8_t delayTimer;
	uint8_t soundTimer;
} cpu_t;

extern cpu_t cpu;

enum {
	OP_MISC = 0x0,
	OP_JMP = 0x1,
	OP_CALL = 0x2,
	OP_SKIP1 = 0x3,
	OP_SKIP2 = 0x4,
	OP_SKIP3 = 0x5,
	OP_MOV_IMM = 0x6,
	OP_ADD_IMM = 0x7,
	OP_MATH = 0x8,
	OP_SKIP4 = 0x9,
	OP_STORE = 0xA,
	OP_JMP_INDEX = 0xB,
	OP_RAND = 0xC,
	OP_DRAW = 0xD,
	OP_IN = 0xE,
	OP_MISC2 = 0xF,
};

extern char* opNames[];

void cpuInit(void);

#ifdef CPU_INTERP
void cpuInterpStep(void);
#endif

#ifdef CPU_JIT
void cpuJitRun(void);
#endif

void unimplemented(uint16_t opcode);

#endif // CPU_H
