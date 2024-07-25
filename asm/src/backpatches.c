#include "backpatches.h"

#include "labels.h"
#include "files.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

backpatch_t* backpatches = NULL;
backpatch_t* lastBackpatch = NULL;

void backpatchHere(uint16_t addr, char* name) {
	if(backpatches == NULL) {
		backpatches = malloc(sizeof(backpatch_t));
		lastBackpatch = backpatches;
	} else {
		lastBackpatch->next = malloc(sizeof(backpatch_t));
		lastBackpatch = lastBackpatch->next;
	}
	lastBackpatch->next = NULL;
	lastBackpatch->address = addr;
	strcpy(lastBackpatch->labelName, name);
}

void fillBackpatches(void) {
	backpatch_t* b = backpatches;
	while(b != NULL) {
		label_t* l = findLabel(b->labelName);
		if(l == NULL) {
			printf("could not find label \"%s\"\n", b->labelName);
			exit(1);
		}

		uint16_t addr = l->address;
		if(littleEndian) {
			addr = (addr << 8) | (addr >> 8);
		}

		*((uint16_t*)(outputFile.buffer + b->address - 0x200)) |= addr;

		b = b->next;
	}
}
