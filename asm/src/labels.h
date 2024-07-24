#ifndef LABELS_H
#define LABELS_H

#include <stdint.h>

#define MAX_LABEL_LEN 64

typedef struct label_struct {
	char name[MAX_LABEL_LEN];
	uint16_t address;
	struct label_struct* next;
} label_t;
extern label_t* labels;

void newLabel(char* name, uint16_t address);
label_t* findLabel(char* name);

#endif // LABELS_H
