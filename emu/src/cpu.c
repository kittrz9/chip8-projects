#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t font[] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,
	0x20, 0x60, 0x20, 0x20, 0x70,
	0xF0, 0x10, 0xF0, 0x80, 0xF0,
	0xF0, 0x10, 0xF0, 0x10, 0xF0,
	0x90, 0x90, 0xF0, 0x10, 0x10,
	0xF0, 0x80, 0xF0, 0x10, 0xF0,
	0xF0, 0x80, 0xF0, 0x90, 0xF0,
	0xF0, 0x10, 0x20, 0x40, 0x40,
	0xF0, 0x90, 0xF0, 0x90, 0xF0,
	0xF0, 0x90, 0xF0, 0x10, 0xF0,
	0xF0, 0x90, 0xF0, 0x90, 0x90,
	0xE0, 0x90, 0xE0, 0x90, 0xE0,
	0xF0, 0x80, 0x80, 0x80, 0xF0,
	0xE0, 0x90, 0x90, 0x90, 0xE0,
	0xF0, 0x80, 0xF0, 0x80, 0xF0,
	0xF0, 0x80, 0xF0, 0x80, 0x80
};

cpu_t cpu;

uint8_t ram[0x1000];

char* opNames[] = {
	"OP_MISC",
	"OP_JMP",
	"OP_CALL",
	"OP_SKIP1",
	"OP_SKIP2",
	"OP_SKIP3",
	"OP_MOV_IMM",
	"OP_ADD_IMM",
	"OP_MATH",
	"OP_SKIP4",
	"OP_STORE",
	"OP_JMP_INDEX",
	"OP_RAND",
	"OP_DRAW",
	"OP_IN",
	"OP_MISC2",
};

void unimplemented(uint16_t opcode) {
	printf("unimplemented opcode %04X\n", opcode);
	exit(1);
}

void cpuInit(void) {
	cpu.pc = 0x200;
	memcpy(ram, font, sizeof(font));

	#ifdef CPU_JIT
	// allocate executable ram for the jit compiler
	#endif
}