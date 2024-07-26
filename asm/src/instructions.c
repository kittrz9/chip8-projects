#include "instructions.h"

#include <stdio.h>
#include <string.h>

#include "backpatches.h"

enum {
	INSTR_CLEAR,
	INSTR_RET,
	INSTR_JMP,
	INSTR_CALL,
	INSTR_SKIP_EQ,
	INSTR_SKIP_NE,
	INSTR_LDV,
	INSTR_ADD,
	INSTR_OR,
	INSTR_AND,
	INSTR_XOR,
	INSTR_SUB,
	INSTR_SUB2,
	INSTR_SHIFT_R,
	INSTR_SHIFT_L,
	INSTR_LDI,
	INSTR_RAND,
	INSTR_DRAW,
	INSTR_SKIP_KEY_DOWN,
	INSTR_SKIP_KEY_UP,
	INSTR_GET_DELAY,
	INSTR_SET_DELAY,
	INSTR_GET_SOUND,
	INSTR_SET_SOUND,
	INSTR_ADD_I,
	INSTR_GET_SPRITE,
	INSTR_BCD,
	INSTR_DUMP_REG,
	INSTR_LOAD_REG,
	INSTR_JMP_I,
	TOTAL_INSTR,
	INSTR_INVALID,
};

const char* instructionNames[] = {
	[INSTR_CLEAR] = "clear",
	[INSTR_RET] = "ret",
	[INSTR_JMP] = "jmp",
	[INSTR_CALL] = "call",
	[INSTR_SKIP_EQ] = "skip_eq",
	[INSTR_SKIP_NE] = "skip_ne",
	[INSTR_LDV] = "ldv",
	[INSTR_ADD] = "add",
	[INSTR_OR] = "or",
	[INSTR_AND] = "and",
	[INSTR_XOR] = "xor",
	[INSTR_SUB] = "sub",
	[INSTR_SUB2] = "sub2",
	[INSTR_SHIFT_R] = "shift_r",
	[INSTR_SHIFT_L] = "shift_l",
	[INSTR_LDI] = "ldi",
	[INSTR_RAND] = "rand",
	[INSTR_DRAW] = "draw",
	[INSTR_SKIP_KEY_DOWN] = "skip_key_down",
	[INSTR_SKIP_KEY_UP] = "skip_key_up",
	[INSTR_GET_DELAY] = "get_delay",
	[INSTR_SET_DELAY] = "set_delay",
	[INSTR_GET_SOUND] = "get_sound",
	[INSTR_SET_SOUND] = "set_sound",
	[INSTR_ADD_I] = "add_i",
	[INSTR_GET_SPRITE] = "get_sprite",
	[INSTR_BCD] = "bcd",
	[INSTR_DUMP_REG] = "dump_reg",
	[INSTR_LOAD_REG] = "load_reg",
	[INSTR_JMP_I] = "jmp_i",
};

uint8_t validInstruction(char* str) {
	for(uint8_t i = 0; i < sizeof(instructionNames)/sizeof(instructionNames[0]); ++i) {
		if(strcmp(str, instructionNames[i]) == 0) {
			return 1;
		}
	}
	return 0;
}

// will affect currentToken
uint16_t processInstruction(token_t** currentToken, uint16_t pc) {
	uint8_t instructionIndex = INSTR_INVALID;
	for(uint8_t i = 0; i < TOTAL_INSTR; ++i) {
		if(strcmp((*currentToken)->str, instructionNames[i]) == 0) {
			instructionIndex = i;
		}
	}
	uint16_t opcode = 0;
	switch(instructionIndex) {
		case INSTR_LDV: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			if((*currentToken)->type == TOKEN_REGISTER) {
				uint8_t y = getTokenReg(*currentToken);
				opcode = 0x8000 | (x << 8) | (y << 4);
			} else if((*currentToken)->type == TOKEN_NUMBER) {
				uint8_t n = getTokenNum(*currentToken);
				opcode = 0x6000 | (x << 8) | n;
			}
			break;
		}
		case INSTR_GET_SPRITE: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			opcode = 0xF029 | (x << 8);
			break;
		}
		case INSTR_DRAW: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			uint8_t y = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			uint8_t n = getTokenNum(*currentToken);
			opcode = 0xD000 | (x << 8) | (y << 4) | n;
			break;
		}
		case INSTR_ADD: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			if((*currentToken)->type == TOKEN_REGISTER) {
				uint8_t y = getTokenReg(*currentToken);
				opcode = 0x8004 | (x << 8) | (y << 4);
			} else if((*currentToken)->type == TOKEN_NUMBER) {
				uint8_t n = getTokenNum(*currentToken);
				opcode = 0x7000 | (x << 8) | n;
			}
			break;
		}
		case INSTR_JMP: {
			*currentToken = tokenNext(*currentToken);
			uint16_t addr = getTokenLabelAddr(*currentToken);
			if(addr == 0) { backpatchHere(pc, (*currentToken)->str + 1); } // skip the @
			opcode = 0x1000 | addr;
			break;
		}
		case INSTR_CLEAR: {
			opcode = 0x00E0;
			break;
		}
		case INSTR_RET: {
			opcode = 0x00EE;
			break;
		}
		case INSTR_CALL: {
			// need to get it to be able to call functions after it to actually be useful
			*currentToken = tokenNext(*currentToken);
			uint16_t addr = getTokenLabelAddr(*currentToken);
			if(addr == 0) { backpatchHere(pc, (*currentToken)->str + 1); } // skip the @
			opcode = 0x2000 | addr;
			break;
		}
		case INSTR_SKIP_EQ: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			if((*currentToken)->type == TOKEN_REGISTER) {
				uint8_t y = getTokenReg(*currentToken);
				opcode = 0x5000 | (x << 8) | (y << 4);
			} else if((*currentToken)->type == TOKEN_NUMBER) {
				uint8_t n = getTokenNum(*currentToken);
				opcode = 0x3000 | (x << 8) | n;
			}
			break;
		}
		case INSTR_SKIP_NE: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			if((*currentToken)->type == TOKEN_REGISTER) {
				uint8_t y = getTokenReg(*currentToken);
				opcode = 0x9000 | (x << 8) | (y << 4);
			} else if((*currentToken)->type == TOKEN_NUMBER) {
				uint8_t n = getTokenNum(*currentToken);
				opcode = 0x4000 | (x << 8) | n;
			}
			break;
		}
		case INSTR_OR: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			uint8_t y = getTokenReg(*currentToken);
			opcode = 0x8001 | (x << 8) | (y << 4);
			break;
		}
		case INSTR_AND: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			uint8_t y = getTokenReg(*currentToken);
			opcode = 0x8002 | (x << 8) | (y << 4);
			break;
		}
		case INSTR_XOR: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			uint8_t y = getTokenReg(*currentToken);
			opcode = 0x8003 | (x << 8) | (y << 4);
			break;
		}
		case INSTR_SUB: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			uint8_t y = getTokenReg(*currentToken);
			opcode = 0x8005 | (x << 8) | (y << 4);
			break;
		}
		case INSTR_SUB2: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			uint8_t y = getTokenReg(*currentToken);
			opcode = 0x8007 | (x << 8) | (y << 4);
			break;
		}
		case INSTR_SHIFT_R: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			uint8_t y = getTokenReg(*currentToken);
			opcode = 0x8006 | (x << 8) | (y << 4);
			break;
		}
		case INSTR_SHIFT_L: {
			*currentToken = tokenNext(*currentToken);
			uint8_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			uint8_t y = getTokenReg(*currentToken);
			opcode = 0x800E | (x << 8) | (y << 4);
			break;
		}
		case INSTR_LDI: {
			*currentToken = tokenNext(*currentToken);
			uint16_t n = getTokenNum(*currentToken);
			opcode = 0xA000 | n;
			break;
		}
		case INSTR_RAND: {
			*currentToken = tokenNext(*currentToken);
			uint16_t x = getTokenReg(*currentToken);
			*currentToken = tokenNext(*currentToken);
			uint16_t n = getTokenNum(*currentToken);
			opcode = 0xC000 | (x << 8) | n;
			break;
		}
		case INSTR_SKIP_KEY_UP: {
			*currentToken = tokenNext(*currentToken);
			uint16_t x = getTokenReg(*currentToken);
			opcode = 0xE09E | (x << 8);
			break;
		}
		case INSTR_SKIP_KEY_DOWN: {
			*currentToken = tokenNext(*currentToken);
			uint16_t x = getTokenReg(*currentToken);
			opcode = 0xE0A1 | (x << 8);
			break;
		}
		case INSTR_GET_DELAY: {
			*currentToken = tokenNext(*currentToken);
			uint16_t x = getTokenReg(*currentToken);
			opcode = 0xF007 | (x << 8);
			break;
		}
		case INSTR_SET_DELAY: {
			*currentToken = tokenNext(*currentToken);
			uint16_t x = getTokenReg(*currentToken);
			opcode = 0xF015 | (x << 8);
			break;
		}
		case INSTR_SET_SOUND: {
			*currentToken = tokenNext(*currentToken);
			uint16_t x = getTokenReg(*currentToken);
			opcode = 0xF00A | (x << 8);
			break;
		}
		case INSTR_ADD_I: {
			*currentToken = tokenNext(*currentToken);
			uint16_t x = getTokenReg(*currentToken);
			opcode = 0xF01E | (x << 8);
			break;
		}
		case INSTR_BCD: {
			*currentToken = tokenNext(*currentToken);
			uint16_t x = getTokenReg(*currentToken);
			opcode = 0xF033 | (x << 8);
			break;
		}
		case INSTR_DUMP_REG: {
			*currentToken = tokenNext(*currentToken);
			uint16_t x = getTokenReg(*currentToken);
			opcode = 0xF055 | (x << 8);
			break;
		}
		case INSTR_LOAD_REG: {
			*currentToken = tokenNext(*currentToken);
			uint16_t x = getTokenReg(*currentToken);
			opcode = 0xF065 | (x << 8);
			break;
		}
		case INSTR_JMP_I: {
			*currentToken = tokenNext(*currentToken);
			uint16_t addr = getTokenLabelAddr(*currentToken);
			if(addr == 0) { backpatchHere(pc, (*currentToken)->str + 1); } // skip the @
			opcode = 0xB000 | addr;
			break;
		}
	}
	return opcode;
}
