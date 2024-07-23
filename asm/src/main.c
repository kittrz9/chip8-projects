#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

#define MAX_STRLEN 64

typedef struct label_struct {
	char name[MAX_STRLEN];
	uint16_t address;
	struct label_struct* next;
} label_t;
label_t* labels = NULL;

label_t* findLabel(char* name) {
	label_t* currentLabel = labels;
	label_t* foundLabel = NULL;
	while(currentLabel != NULL) {
		if(strcmp(currentLabel->name, name) == 0) {
			foundLabel = currentLabel;
			break;
		}
		currentLabel = currentLabel->next;
	}
	return foundLabel;
}

typedef struct line_struct {
	char str[MAX_STRLEN];
	struct line_struct* next;
} line_t;

int main(int argc, char** argv) {
	if(argc != 2) { exit(1); }

	FILE* f = fopen(argv[1], "rb");

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

	// split it into lines
	line_t* lines = malloc(sizeof(line_t));
	lines->next = NULL;

	c = asmFile;
	line_t* currentLine = lines;
	uint8_t lineIndex = 0;
	while(*c != '\0') {
		if(*c == '\n') {
			currentLine->next = malloc(sizeof(line_t));
			currentLine = currentLine->next;
			currentLine->next = NULL;
			lineIndex = 0;
		} else {
			if(lineIndex >= MAX_STRLEN) {
				printf("max line length exceeded\n");
				exit(1);
			}

			currentLine->str[lineIndex] = *c;
			++lineIndex;
		}

		++c;
	}

	//printf("%s\n", asmFile);

	free(asmFile);

	currentLine = lines;
	uint16_t pc = 0x200;
	label_t* lastLabel = labels;
	while(currentLine != NULL) {
		printf("-%s\n", currentLine->str);
		c = currentLine->str;
		if(currentLine->str[0] >= 'a' && currentLine->str[0] <= 'z') {
			printf("new label found\n");
			if(labels == NULL) {
				labels = malloc(sizeof(label_t));
				lastLabel = labels;
			} else {
				lastLabel->next = malloc(sizeof(label_t));
				lastLabel = lastLabel->next;
			}
			lastLabel->next = NULL;
			lastLabel->address = pc;
			uint8_t i = 0;
			while(*c != ':' && *c != '\n' && *c != '\0') {
				lastLabel->name[i] = *c;
				++c;
				++i;
			}
		} else {
			while(*c == ' ' || *c == '\t') {
				++c;
			}
			// really awful way to do this
			// should probably also like do actual lexical analysis or whatever
			if(strncmp(c, "ldv", 3) == 0) {
				// skip to the first argument and the v
				c += 5;
				uint8_t x = hexStrToInt(c, 1);
				++c;
				// should probably make it only ignore a single comma but whatever
				while(*c == ' ' || *c == ',') {
					++c;
				}
				uint16_t opcode;
				if(*c == 'v') {
					++c;
					uint8_t y = hexStrToInt(c, 1);
					opcode = 0x8000 | (x << 8) | (y << 4);
				} else {
					uint8_t immediate = hexStrToInt(c, 2);
					opcode = 0x6000 | (x << 8) | immediate;
				}
				// just printing out the opcodes for now
				// will eventually send these out to a file or something
				printf("%04X\n", opcode);
			} else if(strncmp(c, "get_sprite", 10) == 0) {
				c += 12;
				uint8_t x = hexStrToInt(c, 1);
				uint16_t opcode = 0xF029 | (x << 8);
				printf("%04X\n", opcode);
			} else if(strncmp(c, "add", 3) == 0) {
				c += 5;
				uint8_t x = hexStrToInt(c, 1);
				++c;
				while(*c == ' ' || *c == ',') {
					++c;
				}
				uint16_t opcode;
				if(*c == 'v') {
					++c;
					uint8_t y = hexStrToInt(c, 1);
					opcode = 0x8004 | (x << 8) | (y << 4);
				} else {
					uint8_t immediate = hexStrToInt(c, 2);
					opcode = 0x7000 | (x << 8) | immediate;
				}
				printf("%04X\n", opcode);
			} else if(strncmp(c, "draw", 4) == 0) {
				c += 6;
				uint8_t x = hexStrToInt(c, 1);
				++c;
				while(*c == ' ' || *c == ',') {
					++c;
				}
				uint8_t y = hexStrToInt(c, 1);
				++c;
				while(*c == ' ' || *c == ',') {
					++c;
				}
				uint8_t n = hexStrToInt(c, 1);
				uint16_t opcode = 0xD000 | (x << 8) | (y << 4) | n;
				printf("%04X\n", opcode);
			} else if(strncmp(c, "jmp", 3) == 0) {
				c += 4;
				label_t* jumpLabel = findLabel(c);
				if(jumpLabel == NULL) {
					printf("label \"%s\" not found\n", c);
					exit(1);
				}
				uint16_t opcode = 0x1000 | jumpLabel->address;
				printf("%04X\n", opcode);
			} else {
				printf("unimplemented instruction\n");
			}
			pc += 2;
		}
		currentLine = currentLine->next;
	}

	label_t* currentLabel = labels;
	while(currentLabel != NULL) {
		printf("- name: \"%s\", address, %03X -\n", currentLabel->name, currentLabel->address);
		currentLabel = currentLabel->next;
	}

	return 0;
}
