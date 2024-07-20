#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#include "chunkStrings.h"

size_t romSize;
uint8_t* rom;

typedef struct chunk_struct{
	uint16_t offset;
	uint8_t size;
	chunkString opcodes;
	struct chunk_struct* next;
} chunk_t;

chunk_t* chunks = NULL;

chunk_t* lastChunk = NULL;
chunk_t* chunkCreate(uint16_t offset) {
	chunk_t* c = malloc(sizeof(chunk_t));
	c->offset = offset;
	c->size = 0;
	c->next = NULL;
	chunkStringInit(&c->opcodes);
	if(chunks == NULL) {
		chunks = c;
	} else {
		lastChunk->next = c;
	}
	lastChunk = c;

	return c;
}

void addOpcode(chunk_t* chunk, char* fmt, ...) {
	va_list args;
	va_start(args,fmt);
	va_list args2;
	va_copy(args2, args);
	
	int length = vsnprintf(NULL, 0, fmt, args);
	
	char* str = malloc(length + 1);

	vsprintf(str, fmt, args2);
	va_end(args);
	va_end(args2);
	chunkStringAdd(&chunk->opcodes, str);
	free(str);
}

void unimplemented(chunk_t* chunk, uint16_t opcode) {
	addOpcode(chunk, "(UNIMPLEMENTED OPCODE %04X, SKIPPING)\n", opcode);
}

bool checkIfDisassembled(uint16_t pc) {
	chunk_t* c = chunks;
	while(c != NULL) {
		if(c->size != 0 && pc >= c->offset && pc <= c->offset + c->size) {
			return true;
		}
		c = c->next;
	}
	return false;
}

void disasmChunk(uint16_t pc);

// I don't think there are agreed upon instruction names
// basing this very loosely on the 6502 names
void disasmOpcode(chunk_t* chunk, uint16_t opcode) {
	addOpcode(chunk, "\t");
#define X (opcode>>8 & 0xF)
#define Y (opcode>>4 & 0xF)
#define IMM (opcode & 0xFF)
#define ADDR (opcode & 0xFFF)
	switch(opcode >> 12) {
		case 0:
			switch(opcode & 0xFF) {
				case 0xE0:
					addOpcode(chunk, "clear\n");
					break;
				case 0xEE:
					addOpcode(chunk, "ret\n");
					break;
				default:
					unimplemented(chunk, opcode);
					break;
			}
			break;
		case 1:
			addOpcode(chunk, "jmp %03X\n", ADDR);
			disasmChunk( ADDR);
			break;
		case 2:
			addOpcode(chunk, "call %03X\n", ADDR);
			disasmChunk(ADDR);
			break;
		case 3:
			addOpcode(chunk, "skip_eq V%01X, %02X\n", X, IMM);
			break;
		case 4:
			addOpcode(chunk, "skip_ne V%01X, %02X\n", X, IMM);
			break;
		case 5:
			addOpcode(chunk, "skip_eq V%01X, v%01X\n", X, Y);
			break;
		case 6:
			addOpcode(chunk, "ldv V%01X, %02X\n", X, IMM);
			break;
		case 7:
			addOpcode(chunk, "add V%01X, %02X\n", X, IMM);
			break;
		case 8:
			switch(opcode & 0xF) {
				case 0:
					addOpcode(chunk, "ldv V%01X, V%01X\n", X, Y);
					break;
				case 1:
					addOpcode(chunk, "or V%01X, V%01X\n", X, Y);
					break;
				case 2:
					addOpcode(chunk, "and V%01X, V%01X\n", X, Y);
					break;
				case 3:
					addOpcode(chunk, "xor V%01X, V%01X\n", X, Y);
					break;
				case 4:
					addOpcode(chunk, "add V%01X, V%01X\n", X, Y);
					break;
				case 5:
					addOpcode(chunk, "sub V%01X, V%01X\n", X, Y);
					break;
				case 6:
					addOpcode(chunk, "shift_r V%01X\n", X);
					break;
				case 7:
					// same as 5 but subtracts y from x, stills stores it in x
					// needs a better instruction name
					addOpcode(chunk, "sub2 V%01X, V%01X\n", X, Y);
					break;
				case 0xE:
					addOpcode(chunk, "shift_l V%01X\n", X);
					break;
				default:
					unimplemented(chunk, opcode);
			}
			break;
		case 9:
			addOpcode(chunk, "skip_ne V%01X, v%01X\n", X, Y);
			break;
		case 0xA:
			addOpcode(chunk, "ldi %03X\n", ADDR);
			break;
		case 0xB:
			addOpcode(chunk, "jmp %03X+V0\n", ADDR);
			break;
		case 0xC:
			addOpcode(chunk, "rand V%01X, %02X\n", X, opcode & 0xFF);
			break;
		case 0xD:
			addOpcode(chunk, "draw V%01X, V%01X, %01X\n", X, Y, opcode & 0xF);
			break;
		case 0xE:
			switch(opcode & 0xFF) {
				// could probably get better names for these
				case 0x9E:
					addOpcode(chunk, "skip_key_down V%01X\n", X);
					break;
				case 0xA1:
					addOpcode(chunk, "skip_key_up V%01X\n", X);
					break;
				default:
					unimplemented(chunk, opcode);
					break;
			}
			break;
		case 0xF:
			switch(opcode & 0xFF) {
				case 0x07:
					addOpcode(chunk, "get_delay V%01X\n", X);
					break;
				case 0x0A:
					addOpcode(chunk, "get_key V%01X\n", X);
					break;
				case 0x15:
					addOpcode(chunk, "set_delay V%01X\n", X);
					break;
				case 0x18:
					addOpcode(chunk, "set_sound V%01X\n", X);
					break;
				case 0x29:
					addOpcode(chunk, "add_i V%01X\n", X);
					break;
				case 0x33:
					addOpcode(chunk, "bcd V%01X\n", X);
					break;
				case 0x55:
					addOpcode(chunk, "dump_reg V%01X\n", X);
					break;
				case 0x65:
					addOpcode(chunk, "load_reg V%01X\n", X);
					break;
				default:
					unimplemented(chunk, opcode);
			}
			break;
		default:
			unimplemented(chunk, opcode);
	}
}

void disasmChunk(uint16_t pc) {
	if(pc-0x200 > romSize) { return; }
	if(checkIfDisassembled(pc)) { return; }
	chunk_t* newChunk = chunkCreate(pc);
	while(pc-0x200 < romSize) {
		uint16_t currentOpcode = rom[pc-0x200]<<8 | rom[pc-0x200+1];
		uint16_t lastOpcode = 0;
		if(pc != 0) { // to make sure it doesn't read out of bounds
			lastOpcode = rom[pc-0x200]<<8 | rom[pc-0x200+1];
		}
		disasmOpcode(newChunk, currentOpcode);
		// if the current opcode is a jump and cant be skipped, end chunk
		switch(lastOpcode >> 12) {
			case 3:
			case 4:
			case 5:
			case 9:
				break;
			default:
				if(currentOpcode >> 12 == 1 || currentOpcode >> 12 == 0xB || currentOpcode == 0x00EE) {
					return;
				}
				break;
		}
		pc += 2;
		newChunk->size += 2;
	}
}

int main(int argc, char** argv) {
	if(argc != 2) { exit(1); }
	FILE* f = fopen(argv[1], "rb");
	fseek(f, 0, SEEK_END);
	romSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	printf("size: %04X\n", romSize);

	rom = malloc(romSize);

	fread(rom, romSize, 1, f);

	disasmChunk(0x200);

	chunk_t* c = chunks;
	while(c != NULL) {
		printf("%03X:\n", c->offset);
		printf("%s\n", c->opcodes.str);

		chunk_t* next = c->next;
		free(c->opcodes.str);
		free(c);
		c = c->next;
	}

	free(rom);

	return 0;
}
