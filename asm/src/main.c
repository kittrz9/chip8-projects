#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "lines.h"
#include "labels.h"

uint16_t hexStrToInt(char* str, uint8_t digits) {
	char hexLUT[] = "0123456789abcdef";
	uint16_t value = 0;
	while(digits != 0) {
		if(*str == ' ' || *str == '\n' || *str == ',' || *str == '\0') { break; }
		value <<= 4;
		uint8_t i;
		for(i = 0; i < sizeof(hexLUT)/sizeof(hexLUT[0]); ++i) {
			if(*str == hexLUT[i]) {
				break;
			}
		}
		value |= i;
		++str;
		--digits;
	}
	return value;
}

uint8_t endianCheck(void) {
	uint16_t i = 1;
	return *((uint8_t*)&i);
}

int main(int argc, char** argv) {
	if(argc != 2) { exit(1); }

	uint8_t littleEndian = endianCheck();

	FILE* f = fopen(argv[1], "rb");
	FILE* outputFile = fopen("out.ch8", "wb");

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* asmFile = malloc(size);
	fread(asmFile, size, 1, f);

	// convert it all to lowercase
	// so its easier to process
	char* c = asmFile;
	while(*c != '\0') {
		if(*c >= 'A' && *c <= 'Z') {
			*c += 'a'-'A';
		}
		++c;
	}

	line_t* lines = strToLines(asmFile);

	free(asmFile);

	line_t* currentLine = lines;
	uint16_t pc = 0x200;
	while(currentLine != NULL) {
		printf("-%s\n", currentLine->str);
		// should probably tokenize the line
		c = currentLine->str;
		if(currentLine->str[0] >= 'a' && currentLine->str[0] <= 'z') {
			newLabel(c, pc);
		} else if(*c == ' ' || *c == '\t') {
			c = strtok(c, "\t ");
			uint16_t opcode = 0;
			// really awful way to do this
			// should probably also like do actual lexical analysis or whatever
			if(strncmp(c, "ldv", 3) == 0) {
				// skip to the first argument and the v
				c = strtok(NULL, " ,v");
				uint8_t x = hexStrToInt(c, 1);
				c = strtok(NULL, " ,");
				if(*c == 'v') {
					++c;
					uint8_t y = hexStrToInt(c, 1);
					opcode = 0x8000 | (x << 8) | (y << 4);
				} else {
					uint8_t immediate = hexStrToInt(c, 2);
					opcode = 0x6000 | (x << 8) | immediate;
				}
			} else if(strncmp(c, "get_sprite", 10) == 0) {
				c = strtok(NULL, " ,v");
				uint8_t x = hexStrToInt(c, 1);
				opcode = 0xF029 | (x << 8);
			} else if(strncmp(c, "add", 3) == 0) {
				c = strtok(NULL, " ,v");
				uint8_t x = hexStrToInt(c, 1);
				c = strtok(NULL, " ,");
				if(*c == 'v') {
					++c;
					uint8_t y = hexStrToInt(c, 1);
					opcode = 0x8004 | (x << 8) | (y << 4);
				} else {
					uint8_t immediate = hexStrToInt(c, 2);
					opcode = 0x7000 | (x << 8) | immediate;
				}
			} else if(strncmp(c, "draw", 4) == 0) {
				c = strtok(NULL, " ,v");
				uint8_t x = hexStrToInt(c, 1);
				c = strtok(NULL, " ,v");
				uint8_t y = hexStrToInt(c, 1);
				c = strtok(NULL, " ,v");
				uint8_t n = hexStrToInt(c, 1);
				opcode = 0xD000 | (x << 8) | (y << 4) | n;
			} else if(strncmp(c, "jmp", 3) == 0) {
				c = strtok(NULL, " ,v");
				label_t* jumpLabel = findLabel(c);
				if(jumpLabel == NULL) {
					printf("label \"%s\" not found\n", c);
					exit(1);
				}
				opcode = 0x1000 | jumpLabel->address;
			} else {
				printf("unimplemented instruction\n");
			}
			printf("%04X\n", opcode);

			if(littleEndian) {
				opcode = (opcode << 8) | (opcode >> 8);
			}

			fwrite(&opcode, 2, 1, outputFile);
			pc += 2;
		}
		line_t* next = currentLine->next;
		free(currentLine);
		currentLine = next;
	}

	label_t* currentLabel = labels;
	while(currentLabel != NULL) {
		printf("- name: \"%s\", address, %03X -\n", currentLabel->name, currentLabel->address);
		label_t* next = currentLabel->next;
		free(currentLabel);
		currentLabel = next;
	}

	return 0;
}
