#ifndef CHUNKSTRING_H
#define CHUNKSTRING_H

#include <stddef.h>

typedef struct {
	char* str;
	size_t length;
	size_t size;
} chunkString;

void chunkStringInit(chunkString* chunkStr);
void chunkStringAdd(chunkString* chunkStr, char* str);

#endif // CHUNKSTRING_H
