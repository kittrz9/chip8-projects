#include "chunk.h"

#include <stdlib.h>
#include <stdio.h>

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

