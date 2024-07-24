#include "labels.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

label_t* labels = NULL;
label_t* lastLabel = NULL;

void newLabel(char* name, uint16_t address) {
	if(labels == NULL) {
		labels = malloc(sizeof(label_t));
		lastLabel = labels;
	} else {
		lastLabel->next = malloc(sizeof(label_t));
		lastLabel = lastLabel->next;
	}
	lastLabel->next = NULL;
	lastLabel->address = address;
	uint8_t i = 0;
	while(*name != ':' && *name != '\n' && *name != '\0') {
		lastLabel->name[i] = *name;
		++name;
		++i;
	}
	lastLabel->name[i] = '\0';
}

label_t* findLabel(char* name) {
	label_t* currentLabel = labels;
	while(currentLabel != NULL) {
		if(strcmp(currentLabel->name, name) == 0) {
			return currentLabel;
		}
		currentLabel = currentLabel->next;
	}
	return NULL;
}
