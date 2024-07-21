#ifndef CHUNK_H
#define CHUNK_H

#include "chunkStrings.h"

#include <stdarg.h>
#include <stdint.h>

typedef struct chunk_struct{
	uint16_t offset;
	uint8_t size;
	chunkString opcodes;
	struct chunk_struct* next;
} chunk_t;

extern chunk_t* chunks;

chunk_t* chunkCreate(uint16_t offset);
void addOpcode(chunk_t* chunk, char* fmt, ...);

#endif // CHUNK_H
