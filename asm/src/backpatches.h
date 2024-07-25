#ifndef BACKPATCHES_H
#define BACKPATCHES_H

#include <stdint.h>

#include "labels.h"

typedef struct backpatch_struct{
	uint16_t address;
	char labelName[MAX_LABEL_LEN];
	struct backpatch_struct* next;
} backpatch_t;

void backpatchHere(uint16_t addr, char* name);
void fillBackpatches(void);

#endif // BACKPATCHES_H
