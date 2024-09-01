#ifdef CPU_JIT
#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#include "io.h"

// arbitrary size
#define EXEC_BUFFER_SIZE 4096

typedef struct {
	uint8_t* instructions;
	size_t length; // could probably have a better name, since this is how many bytes have been added to the buffer
} execBuffer_t;

execBuffer_t mainBuffer; // could probably have extra buffers later on

void execBufferInit(execBuffer_t* buffer) {
	buffer->instructions = mmap(NULL, EXEC_BUFFER_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	buffer->length = 0;
}

// x86 opcodes have to be encoded with this because of dumb endianness shenanigans
// I am aware I could use the 16 bit functions if I put them in backwards, but I don't want to
void execBufferAddData(execBuffer_t* buffer, char* data, size_t size) {
	uint8_t* ptr = buffer->instructions + buffer->length;
	buffer->length += size;
	memcpy(ptr, data, size);
}

void execBufferAdd8(execBuffer_t* buffer, uint8_t byte) {
	uint8_t* ptr = (uint8_t*)(buffer->instructions + buffer->length);
	*ptr = byte;
	++buffer->length;
}

void execBufferAdd16(execBuffer_t* buffer, uint16_t data) {
	uint16_t* ptr = (uint16_t*)(buffer->instructions + buffer->length);
	*ptr = data;
	buffer->length += 2;
}

void execBufferAdd32(execBuffer_t* buffer, uint32_t data) {
	uint32_t* ptr = (uint32_t*)(buffer->instructions + buffer->length);
	*ptr = data;
	buffer->length += 4;
}

void execBufferAdd64(execBuffer_t* buffer, uint64_t data) {
	uint64_t* ptr = (uint64_t*)(buffer->instructions + buffer->length);
	*ptr = data;
	buffer->length += 8;
}

void execBufferRun(execBuffer_t* buffer) {
	((void(*)(void))buffer->instructions)();
	buffer->length = 0;
}

void execBufferDebug(execBuffer_t* buffer) {
	for(size_t i = 0; i < buffer->length; ++i) {
		printf("%02X", buffer->instructions[i]);
	}
	printf("\n");
}

// not gonna bother writing this in assembly, should probably move this to the io files
void draw(uint8_t x, uint8_t y, uint8_t h) {
	/*uint8_t xPos = cpu.v[x] % 64;
	uint8_t yPos = cpu.v[y] % 64;
	uint8_t* source = &ram[cpu.i];
	cpu.v[0xF] = 0;
	for(uint8_t i = 0; i < h; ++i) {
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
	}*/
	screenUpdate();
}

void cpuJitInit(void) {
	execBufferInit(&mainBuffer);
	fbPixels[0] = 0xF0;
}

void cpuJitRun(void) {
#define X (op>>8 & 0xF)
#define Y (op>>4 & 0xF)
#define IMM (op & 0xFF)
#define ADDR (op & 0xFFF)
	uint8_t readingOps = 1; // set to 0 when encountering a conditional jmp
	while(readingOps) {
		uint16_t op = ram[cpu.pc]<<8 | ram[cpu.pc+1];
		//printf("%x: %x\n", cpu.pc, op);
		switch(op >> 12) {
			case OP_MISC:
				switch(op&0xFF) {
					case 0xE0:
						// there's probably an easier way to do this, but idk if there's a simpler way to just call a direct 64 bit address
						// mov rax, screenClear
						execBufferAddData(&mainBuffer, "\x48\xb8", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)screenClear);
						// call rax
						execBufferAddData(&mainBuffer, "\xff\xd0", 2);
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
				// cpu.v[X] = IMM
				// 64 bit specific
				// probably a much better way of doing this, but I don't want to dig into how x86 instructions are encoded right now
				// mov rax, cpu.v + x
				execBufferAddData(&mainBuffer, "\x48\xb8", 2);
				execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + X); // should put the pointer to the array in there
				// mov byte [rax], IMM
				execBufferAddData(&mainBuffer, "\xc6\x00", 2);
				execBufferAdd8(&mainBuffer, IMM);
				break;
			case OP_ADD_IMM:
				// cpu.v[X] += IMM;
				// mov rax, byte[cpu.v+x]
				//48a16969000000000000
				execBufferAdd8(&mainBuffer, 0xa0);
				execBufferAdd64(&mainBuffer, (uint64_t)cpu.v+X);
				// add al, IMM
				execBufferAdd8(&mainBuffer, 0x04);
				execBufferAdd8(&mainBuffer, IMM);
				// mov byte[cpu.v+x], al
				execBufferAdd8(&mainBuffer, 0xa2);
				execBufferAdd64(&mainBuffer, (uint64_t)cpu.v+X);
				break;
			case OP_STORE:
				// cpu.i = ADDR;
				// mov rax, &cpu.i
				execBufferAddData(&mainBuffer, "\x48\xb8", 2);
				execBufferAdd64(&mainBuffer, (uint64_t)&cpu.i);
				// mov word [rax], ADDR
				execBufferAddData(&mainBuffer, "\x66\xc7\x00", 3);
				execBufferAdd16(&mainBuffer, ADDR);
				break;
			case OP_MATH:
				switch(op&0xF) {
					case 0:
						//cpu.v[X] = cpu.v[Y];
						// mov al, byte [cpu.v + Y]
						execBufferAdd8(&mainBuffer, 0xa0);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + Y);
						// mov byte [cpu.v + X], al
						execBufferAdd8(&mainBuffer, 0xa2);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + X);
						break;
					case 2:
						// cpu.v[X] &= cpu.v[Y];
						// mov al, byte [cpu.v + X]
						execBufferAdd8(&mainBuffer, 0xa0);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + X);
						// mov rbx, cpu.v + Y
						execBufferAddData(&mainBuffer, "\x48\xbb", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + Y);
						// and al, byte [rbx]
						execBufferAddData(&mainBuffer, "\x22\x03", 2);
						break;
					case 5:
						//cpu.v[0xF] = cpu.v[X] <= cpu.v[Y];
						//cpu.v[X] -= cpu.v[Y];

						// xor cl, cl
						execBufferAddData(&mainBuffer, "\x30\xc9", 2);
						// mov dl, 1
						execBufferAddData(&mainBuffer, "\xb2\x01", 2);
						// mov al, byte[cpu.v + Y]
						execBufferAdd8(&mainBuffer, 0xa0);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + Y);
						// mov bl, al
						execBufferAddData(&mainBuffer, "\x88\xc3", 2);
						// mov al, byte[cpu.v + X]
						execBufferAdd8(&mainBuffer, 0xa0);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + X);
						// sub al, bl
						execBufferAddData(&mainBuffer, "\x66\x29\xd8", 3);
						// cmovge cl, dl ; might be the wrong condition
						execBufferAddData(&mainBuffer, "\x0f\x4d\xca", 3);
						// mov rbx, cpu.v + X
						execBufferAddData(&mainBuffer, "\x48\xbb", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + X);
						// mov byte[rbx], al
						execBufferAddData(&mainBuffer, "\x88\x03", 2);
						// mov rbx, cpu.v + 0xF
						execBufferAddData(&mainBuffer, "\x48\xbb", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + 0xF);
						// mov byte[rbx], cl
						execBufferAddData(&mainBuffer, "\x88\x0b", 2);
						break;
					case 7:
						//cpu.v[0xF] = cpu.v[Y] >= cpu.v[X];
						//cpu.v[X] = cpu.v[Y] - cpu.v[X];

						// xor cl, cl
						execBufferAddData(&mainBuffer, "\x30\xc9", 2);
						// mov dl, 1
						execBufferAddData(&mainBuffer, "\xb2\x01", 2);
						// mov al, byte[cpu.v + Y]
						execBufferAdd8(&mainBuffer, 0xa0);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + Y);
						// mov bl, al
						execBufferAddData(&mainBuffer, "\x88\xc3", 2);
						// mov al, byte[cpu.v + X]
						execBufferAdd8(&mainBuffer, 0xa0);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + X);
						// sub al, bl
						execBufferAddData(&mainBuffer, "\x66\x29\xd8", 3);
						// cmovle cl, dl ; might be the wrong condition
						execBufferAddData(&mainBuffer, "\x0f\x4e\xca", 3);
						// mov rbx, cpu.v + X
						execBufferAddData(&mainBuffer, "\x48\xbb", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + X);
						// mov byte[rbx], al
						execBufferAddData(&mainBuffer, "\x88\x03", 2);
						// mov rbx, cpu.v + 0xF
						execBufferAddData(&mainBuffer, "\x48\xbb", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v + 0xF);
						// mov byte[rbx], cl
						execBufferAddData(&mainBuffer, "\x88\x0b", 2);
						break;
					case 0xE:
						//cpu.v[0xF] = cpu.v[X] >> 7;
						//cpu.v[X] <<= 1;

						// xor bl, bl
						execBufferAddData(&mainBuffer, "\x30\xdb", 2);
						// mov cl, 1
						execBufferAddData(&mainBuffer, "\xb1\x01", 2);
						// mov al, byte[cpu.v + X]
						execBufferAdd8(&mainBuffer, 0xa0);
						execBufferAdd64(&mainBuffer, (uint64_t)(cpu.v + X));
						// sal al, 1
						execBufferAddData(&mainBuffer, "\xd0\xf0", 2);
						// cmovc bl, cl
						execBufferAddData(&mainBuffer, "\x0f\x42\xd9", 3);
						break;
					default:
						unimplemented(op);
				}
				break;
			case OP_MISC2:
				switch(op&0xFF) {
					case 0x29:
						//cpu.i = cpu.v[X] * 5;

						// xor ax, ax
						execBufferAddData(&mainBuffer, "\x66\x31\xc0", 3);
						// mov rbx, cpu.v
						execBufferAddData(&mainBuffer, "\x48\xbb", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)cpu.v);
						// add rbx, X
						execBufferAddData(&mainBuffer, "\x48\x83\xc3", 3);
						execBufferAdd8(&mainBuffer, X);
						// mov al, byte [rbx]
						execBufferAddData(&mainBuffer, "\x8a\x03", 2);
						// mov cl, 5
						execBufferAddData(&mainBuffer, "\xb1\x05", 2);
						// mul cl
						execBufferAddData(&mainBuffer, "\xf6\xe1", 2);
						// mov rbx, &cpu.i
						execBufferAddData(&mainBuffer, "\x48\xbb", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)&cpu.i);
						// mov word[rbx], ax
						execBufferAddData(&mainBuffer, "\x66\x89\x03", 3);
						break;
					case 0x33:
						//uint8_t value = cpu.v[X];
						//for(uint8_t i = 0; i < 3; ++i) {
						//	uint8_t digit = value % 10;
						//	ram[cpu.i + (2-i)] = digit;
						//	value = (value - digit)/10;
						//}

						// could probably do some loop unrolling shenanigans with this
						//
						//    xor rax, rax
						execBufferAddData(&mainBuffer, "\x48\x31\xc0", 3);
						//    mov al, byte [cpu.v + x]
						execBufferAdd8(&mainBuffer, 0xa0);
						execBufferAdd64(&mainBuffer, (uint64_t)(cpu.v + X));
						//    mov rsi, rax
						execBufferAddData(&mainBuffer, "\x48\x89\xc6", 3);
						//    mov cl, 3
						execBufferAddData(&mainBuffer, "\xb1\x03", 2);
						//    mov rbx, (ram + 2) ; also dec each loop
						execBufferAddData(&mainBuffer, "\x48\xbb", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)(ram + 2));
						//    mov rdx, &cpu.i
						execBufferAddData(&mainBuffer, "\x48\xba", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)(&cpu.i));
						//    movzx rax, word[rdx]
						execBufferAddData(&mainBuffer, "\x48\x0f\xb7\x02", 4);
						//    add rbx, rax
						execBufferAddData(&mainBuffer, "\x48\x01\xc3", 3);
						//    mov rdi, 10 ; why can't I use div with an immediate value
						execBufferAddData(&mainBuffer, "\x48\xc7\xc7\x0a\x00\x00\x00", 7);
						//    xor rdx, rdx ; messes up the later div becauase funny x86
						execBufferAddData(&mainBuffer, "\x48\x31\xd2", 3);
						//loop:
						//    mov rax, rsi
						execBufferAddData(&mainBuffer, "\x48\x89\xf0", 3);
						//    div edi ; rdx has remainder
						execBufferAddData(&mainBuffer, "\xf7\xf7", 2);
						//    mov byte [rbx], dl
						execBufferAddData(&mainBuffer, "\x88\x13", 2);
						//    sub rsi, rdx
						execBufferAddData(&mainBuffer, "\x48\x29\xd6", 3);
						//    mov rax, rsi
						execBufferAddData(&mainBuffer, "\x48\x89\xf0", 3);
						//    xor rdx, rdx ; have to clear rdx again beacuse funny x86
						execBufferAddData(&mainBuffer, "\x48\x31\xd2", 3);
						//    div edi ; actual div by 10 in rax
						execBufferAddData(&mainBuffer, "\xf7\xf7", 2);
						//    mov rsi, rax
						execBufferAddData(&mainBuffer, "\x48\x89\xc6", 3);
						//    dec rbx
						execBufferAddData(&mainBuffer, "\x48\xff\xcb", 3);
						//    dec cl
						execBufferAddData(&mainBuffer, "\xfe\xc9", 2);
						//    jnz loop (-23) ; could be wrong, I haven't tested this
						execBufferAddData(&mainBuffer, "\x75\xe4", 2);
						break;
					case 0x65:
						//for(uint8_t i = 0; i <= X; ++i) {
						//cpu.v[i] = ram[cpu.i + i];
						//}

						//   mov cl, X+1
						execBufferAdd8(&mainBuffer, 0xb1);
						execBufferAdd8(&mainBuffer, X);
						//   mov rbx, ram + X
						execBufferAddData(&mainBuffer, "\x48\xbb", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)(ram + X));
						//   mov rdx, &cpu.i
						execBufferAddData(&mainBuffer, "\x48\xba", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)(&cpu.i));
						//   movzx rax, word[rdx]
						execBufferAddData(&mainBuffer, "\x0f\xb6\x02", 3);
						//   add rbx, rax
						execBufferAddData(&mainBuffer, "\x48\x01\xc3", 3);
						//   mov rax, cpu.v + X
						execBufferAddData(&mainBuffer, "\x48\xb8", 2);
						execBufferAdd64(&mainBuffer, (uint64_t)(cpu.v + X));
						//loop:
						//   mov dl, byte[rbx]
						execBufferAddData(&mainBuffer, "\x8a\x13", 2);
						//   mov byte[rax], dl
						execBufferAddData(&mainBuffer, "\x88\x10", 2);
						//   dec rbx
						execBufferAddData(&mainBuffer, "\x48\xff\xcb", 3);
						//   dec rax
						execBufferAddData(&mainBuffer, "\x48\xff\xc8", 3);
						//   dec cl
						execBufferAddData(&mainBuffer, "\xfe\xc9", 2);
						//   jnz loop
						execBufferAddData(&mainBuffer, "\x75\xf2", 2);
						break;
					default:
						unimplemented(op);
				}
				break;
			case OP_DRAW:
				// it seems to break if I try to call screenUpdate in this context, I have no clue why
				// running it with -fsanitize=address makes it not have this issue, and -fsanitize=undefined doesn't seem to show any relevant errors
				// going to just force it to interpret this instruction for now until I can figure this out

				// mov edi, X
				/*execBufferAdd8(&mainBuffer, 0xbf);
				execBufferAdd32(&mainBuffer, (uint32_t)X);
				// mov esi, Y
				execBufferAdd8(&mainBuffer, 0xbe);
				execBufferAdd32(&mainBuffer, (uint32_t)Y);
				// mov edx, op&0xF
				execBufferAdd8(&mainBuffer, 0xba);
				execBufferAdd32(&mainBuffer, (uint32_t)(op&0xF));
				// mov rax, draw
				execBufferAddData(&mainBuffer, "\x48\xb8", 2);
				execBufferAdd64(&mainBuffer, (uint64_t)draw);
				// call rax
				execBufferAddData(&mainBuffer, "\xff\xd0", 2);*/

				execBufferAdd8(&mainBuffer, 0xc3); // ret
				execBufferRun(&mainBuffer);
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
				readingOps = 0;
				break;
			case OP_SKIP1:
				// need to execute everything before being able to know whether to skip or not
				execBufferAdd8(&mainBuffer, 0xc3); // ret
				execBufferRun(&mainBuffer);

				if(cpu.v[X] == IMM) {
					cpu.pc += 2;
				}
				readingOps = 0;
				break;
			case OP_SKIP2:
				execBufferAdd8(&mainBuffer, 0xc3); // ret
				execBufferRun(&mainBuffer);

				if(cpu.v[X] != IMM) {
					cpu.pc += 2;
				}
				readingOps = 0;
				break;
			case OP_JMP_INDEX:
				execBufferAdd8(&mainBuffer, 0xc3); // ret
				execBufferRun(&mainBuffer);

				cpu.pc = cpu.v[0] + ADDR - 2;
				readingOps = 0;
				break;
			default:
				unimplemented(op);
		}
		cpu.pc += 2;
	}
}

#endif // CPU_JIT
