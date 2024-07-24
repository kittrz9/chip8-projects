#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "lines.h"
#include "labels.h"
#include "tokens.h"
#include "instructions.h"

uint8_t endianCheck(void) {
	uint16_t i = 1;
	return *((uint8_t*)&i);
}

uint8_t littleEndian = 0;
void outputOpcode(FILE* f, uint16_t opcode) {
	if(littleEndian) {
		opcode = (opcode << 8) | (opcode >> 8);
	}

	fwrite(&opcode, 2, 1, f);
}

int main(int argc, char** argv) {
	if(argc != 2) { exit(1); }

	littleEndian = endianCheck();

	FILE* f = fopen(argv[1], "rb");
	FILE* outputFile = fopen("out.ch8", "wb");

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* asmFile = malloc(size+1);
	fread(asmFile, size, 1, f);
	asmFile[size] = '\0';

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
		token_t* tokens = tokenize(currentLine->str);
		token_t* currentToken = tokens;
		while(currentToken != NULL) {
			printf("%i \"%s\"\n", currentToken->type, currentToken->str);

			switch(currentToken->type) {
				case TOKEN_LABEL:
					newLabel(currentToken->str, pc);
					break;
				case TOKEN_INSTRUCTION: {
					uint16_t opcode = processInstruction(&currentToken); // autoadvances currentToken
					printf("%04X\n", opcode);
					outputOpcode(outputFile, opcode);
					pc += 2;
					break;
				}
				default:
					printf("syntax error lmao\n");
					exit(1);
			}

			currentToken = tokenNext(currentToken);
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
