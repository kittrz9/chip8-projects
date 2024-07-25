#ifndef FILES_H
#define FILES_H

#include <stdint.h>
#include <stddef.h>

extern uint8_t littleEndian;

char* readFile(char* filename);

// should probably rename this to avoid confusion between stdio FILEs
typedef struct {
	uint8_t* buffer;
	size_t size;
	size_t length;
} file_t;

extern file_t outputFile;

void outputAddWord(uint16_t data);
void outputSave(void);

#endif // FILES_H
