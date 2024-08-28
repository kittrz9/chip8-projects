#ifdef CPU_JIT
#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#include "io.h"

// arbitrary size
#define EXEC_BUFFER_SIZE 4096

typedef void (*func)(void);

typedef struct {
	uint8_t* instructions;
	size_t length; // could probably have a better name, since this is how many bytes have been added to the buffer
} execBuffer_t;

execBuffer_t mainBuffer; // could probably have extra buffers later on

void execBufferInit(execBuffer_t* buffer) {
	buffer->instructions = mmap(NULL, EXEC_BUFFER_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	buffer->length = 0;
}

// data put in as a string so it's easier to input 
void execBufferAdd(execBuffer_t* buffer, char* data, size_t size) {
	uint8_t* ptr = buffer->instructions + buffer->length;
	buffer->length += size;
	memcpy(ptr, data, size);
}

void cpuJitInit(void) {
	execBufferInit(&mainBuffer);
	// mov eax, 0x69; ret
	execBufferAdd(&mainBuffer, "\xb8\x69\x00\x00\x00\xc3", 6);
	// function pointer shenanigans
	printf("%x\n", ((int(*)(void))mainBuffer.instructions)());
}

void cpuJitRun(void) {
	printf("jit unimplemented\n");
	exit(1);
}

#endif // CPU_JIT
