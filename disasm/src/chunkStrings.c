#include "chunkStrings.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define INITIAL_SIZE 32

void chunkStringInit(chunkString* chunkStr) {
	chunkStr->length = 0;
	chunkStr->str = malloc(INITIAL_SIZE);
	chunkStr->size = INITIAL_SIZE;
	chunkStr->str[0] = '\0';
}

void chunkStringAdd(chunkString* chunkStr, char* str) {
	size_t length = strlen(str);
	if(length + chunkStr->length > chunkStr->size) {
		chunkStr->str = realloc(chunkStr->str, chunkStr->size*2);
		chunkStr->size = chunkStr->size*2;
	}

	strcpy(chunkStr->str + chunkStr->length, str);
	chunkStr->length += length;
	chunkStr->str[chunkStr->length] = '\0';

	return;
}
