#ifdef CPU_INTERP
#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"

void cpuInterpStep(void) {
	uint16_t op = ram[cpu.pc]<<8 | ram[cpu.pc+1];
#define X (op>>8 & 0xF)
#define Y (op>>4 & 0xF)
#define IMM (op & 0xFF)
#define ADDR (op & 0xFFF)
	/*printf("\n\npc: %04X, op: %04X \"%s\"\n", cpu.pc, op, opNames[op>>12]);
	for(uint8_t i = 0; i < 0x10; ++i) {
		printf("V%X: %02X, ", i, cpu.v[i]);
	}
	printf("\ni: %04X\n", cpu.i);*/
	switch(op >> 12) {
		case OP_MISC:
			switch(op&0xFF) {
				case 0xE0:
					screenClear();
					break;
				case 0xEE:
					--cpu.sp;
					cpu.pc = cpu.stack[cpu.sp];
					break;
				default:
					unimplemented(op);
			}
			break;
		case OP_JMP:
			cpu.pc = ADDR - 2;
			break;
		case OP_CALL:
			cpu.stack[cpu.sp] = cpu.pc;
			++cpu.sp;
			cpu.pc = ADDR - 2;
			break;
		case OP_MOV_IMM:
			cpu.v[X] = IMM;
			break;
		case OP_MATH:
			switch(op&0xF) {
				case 0:
					cpu.v[X] = cpu.v[Y];
					break;
				case 1:
					cpu.v[X] |= cpu.v[Y];
					break;
				case 2:
					cpu.v[X] &= cpu.v[Y];
					break;
				case 3:
					cpu.v[X] ^= cpu.v[Y];
				case 4:
					cpu.v[0xF] = ((uint16_t)cpu.v[X] + (uint16_t)cpu.v[Y]) > 255;
					cpu.v[X] += cpu.v[Y];
					break;
				case 5:
					cpu.v[0xF] = cpu.v[X] <= cpu.v[Y];
					cpu.v[X] -= cpu.v[Y];
					break;
				case 6:
					cpu.v[0xF] = cpu.v[X] & 1;
					cpu.v[X] >>= 1;
					break;
				case 7:
					cpu.v[0xF] = cpu.v[Y] >= cpu.v[X];
					cpu.v[X] = cpu.v[Y] - cpu.v[X];
					break;
				case 0xE:
					cpu.v[0xF] = cpu.v[X] >> 7;
					cpu.v[X] <<= 1;
					break;
				default:
					unimplemented(op);
			}
			break;
		case OP_ADD_IMM:
			cpu.v[X] += IMM;
			break;
		case OP_STORE:
			cpu.i = ADDR;
			break;
		case OP_MISC2:
			switch(op&0xFF) {
				case 0x07:
					cpu.v[X] = cpu.delayTimer;
					break;
				case 0x15:
					cpu.v[X] = cpu.soundTimer;
					break;
				case 0x29:
					cpu.i = cpu.v[X] * 5;
					break;
				// BCD
				case 0x33: {
					uint8_t value = cpu.v[X];
					for(uint8_t i = 0; i < 3; ++i) {
						uint8_t digit = value % 10;
						ram[cpu.i + (2-i)] = digit;
						value = (value - digit)/10;
					}
					break;
				}
				case 0x65:
					for(uint8_t i = 0; i <= X; ++i) {
						cpu.v[i] = ram[cpu.i + i];
					}
					break;
				default:
					unimplemented(op);
			}
			break;
		case OP_DRAW:
			{
				uint8_t xPos = cpu.v[X] % 64;
				uint8_t yPos = cpu.v[Y] % 64;
				uint8_t* source = &ram[cpu.i];
				cpu.v[0xF] = 0;
				for(uint8_t i = 0; i < (op & 0xF); ++i) {
					for(uint8_t j = 0; j < 8; ++j) {
						uint32_t* target = &fbPixels[xPos + yPos*FRAME_WIDTH];
						uint32_t original = *target;
						uint8_t bit = (*source >> (7-j)) & 1;
						*target = ((0xFFFFFF00 * bit) ^ original)|0xFF;
						if(*target == 0xFF && original != 0xFF) {
							cpu.v[0xF] = 1;
						}
						xPos = (xPos+1) % 64;
					}
					++source;
					yPos = (yPos+1) % 32;
					xPos -= 8;
				}
				screenUpdate();
				//SDL_Delay(150);
			}
			break;
		case OP_SKIP1:
			if(cpu.v[X] == IMM) {
				cpu.pc += 2;
			}
			break;
		case OP_SKIP2:
			if(cpu.v[X] != IMM) {
				cpu.pc += 2;
			}
			break;
		case OP_SKIP3:
			if(cpu.v[X] == cpu.v[Y]) {
				cpu.pc += 2;
			}
			break;
		case OP_SKIP4:
			if(cpu.v[X] != cpu.v[Y]) {
				cpu.pc += 2;
			}
			break;
		case OP_RAND:
			cpu.v[X] = rand() & IMM;
			break;
		case OP_IN:
			printf("input unimplemented: %04X\n", op);
			break;
		case OP_JMP_INDEX:
			cpu.pc = cpu.v[0] + ADDR - 2;
			break;
		default:
			unimplemented(op);
	}
	cpu.pc += 2;
	--cpu.delayTimer;
}
#endif // CPU_INTERP
