#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "lines.h"
#include "labels.h"
#include "tokens.h"
#include "instructions.h"
#include "files.h"
#include "backpatches.h"

int main(int argc, char** argv) {
	if(argc != 2) { exit(1); }

	char* asmFile = readFile(argv[1]);

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
					uint16_t opcode = processInstruction(&currentToken, pc); // autoadvances currentToken
					printf("%04X\n", opcode);
					outputAddWord(opcode);
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

	fillBackpatches();

	label_t* currentLabel = labels;
	while(currentLabel != NULL) {
		printf("- name: \"%s\", address, %03X -\n", currentLabel->name, currentLabel->address);
		label_t* next = currentLabel->next;
		free(currentLabel);
		currentLabel = next;
	}

	outputSave();

	return 0;
}
