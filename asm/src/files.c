#include "files.h"

#include <stdio.h>
#include <stdlib.h>

file_t outputFile;

uint8_t littleEndian = 0;
uint8_t endianCheck(void) {
	uint16_t i = 1;
	return *((uint8_t*)&i);
}

char* readFile(char* filename) {
	// do endian check here since this should only be ran once
	littleEndian = endianCheck();

	FILE* f = fopen(filename, "rb");

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* str = malloc(size+1);
	fread(str, size, 1, f);
	str[size] = '\0';
	fclose(f);

	return str;
}

#define FILE_INITIAL_SIZE 32
void outputAddWord(uint16_t opcode) {
	if(littleEndian) {
		opcode = (opcode << 8) | (opcode >> 8);
	}

	if(outputFile.size == 0) {
		outputFile.buffer = malloc(FILE_INITIAL_SIZE);
		outputFile.size = FILE_INITIAL_SIZE;
	}

	if(outputFile.length+2 > outputFile.size) {
		outputFile.buffer = realloc(outputFile.buffer, outputFile.size*2);
		outputFile.size = outputFile.size*2;
	}

	// could probably just change the buffer's type to uint16_t
	// since as of now all you can add are words
	// but I might eventually allow data to be added with like db or dq or whatever like nasm does
	*((uint16_t*)(outputFile.buffer + outputFile.length)) = opcode;
	outputFile.length += 2;
}

void outputSave(void) {
	FILE* f = fopen("out.ch8", "wb");

	fwrite(outputFile.buffer, outputFile.length, 1, f);

	fclose(f);
}
