#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#define UNIMPLEMENTED printf("unimplemented opcode %04X\n", op);exit(1);

#define FRAME_WIDTH 64
#define FRAME_HEIGHT 32

#define FRAME_SCALE 10

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

struct {
	uint8_t v[0x10];
	uint16_t i;
	uint16_t pc;
	uint16_t stack[16];
	uint8_t sp;
	uint8_t delayTimer;
	uint8_t soundTimer;
} cpu;

uint8_t ram[0x1000];

SDL_Surface* frameBuffer;
SDL_Surface* windowSurface;
SDL_Window* w;

uint32_t* fbPixels = NULL;

void clearScreen(void) {
	for(uint16_t i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; ++i) {
		fbPixels[i] = 0xFF;
	}
	return;
}

void updateScreen(void) {
	SDL_BlitScaled(frameBuffer, &(SDL_Rect){0,0,FRAME_WIDTH,FRAME_HEIGHT}, windowSurface, &(SDL_Rect){0,0,FRAME_WIDTH*FRAME_SCALE,FRAME_HEIGHT*FRAME_SCALE});
	SDL_UpdateWindowSurface(w);
}

int main(int argc, char** argv) {
	if(argc != 2) {
		exit(1);
	}

	FILE* f = fopen(argv[1], "rb");
	if(!f) {
		exit(1);
	}

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	printf("%lu bytes\n", size);

	fread(ram+0x200, size, 1, f);

	fclose(f);

	memcpy(ram, font, sizeof(font));

	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		exit(1);
	}

	w = SDL_CreateWindow("asdfasdfsadf", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, FRAME_WIDTH * FRAME_SCALE, FRAME_HEIGHT * FRAME_SCALE, 0); 
	frameBuffer = SDL_CreateRGBSurface(0, FRAME_WIDTH, FRAME_HEIGHT, 32, 0xFF000000, 0xFF0000, 0xFF00, 0xFF);
	windowSurface = SDL_GetWindowSurface(w);

	fbPixels = (uint32_t*)frameBuffer->pixels;

	clearScreen();

	updateScreen();

	cpu.pc = 0x200;

	while(1) {
		// probably not the best for performance to poll every time an instruction is ran
		// but polling every frame lets things that run in an infinite loop not close
		SDL_Event e;
		SDL_PollEvent(&e);
		if(e.type == SDL_QUIT) { 
			break;
		}
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
						clearScreen();
						break;
					case 0xEE:
						--cpu.sp;
						cpu.pc = cpu.stack[cpu.sp];
						break;
					default:
						UNIMPLEMENTED
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
						UNIMPLEMENTED
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
						UNIMPLEMENTED
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
					updateScreen();
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
			default:
				UNIMPLEMENTED
		}
		cpu.pc += 2;
		--cpu.delayTimer;
	}

	SDL_FreeSurface(frameBuffer);
	SDL_DestroyWindowSurface(w);
	SDL_DestroyWindow(w);

	return 0;
}
