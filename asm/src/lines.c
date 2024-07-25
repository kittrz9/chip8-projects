#include "lines.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

line_t* strToLines(char* str) {
	line_t* lines = malloc(sizeof(line_t));
	lines->next = NULL;
	line_t* currentLine = lines;
	uint16_t lineIndex = 0;
	while(*str != '\0') {
		// get rid of comments
		if(*str == ';') {
			while(*str != '\n') { ++str; }
		}
		if(*str == '\n') {
			currentLine->str[lineIndex] = '\0';
			currentLine->next = malloc(sizeof(line_t));
			currentLine = currentLine->next;
			currentLine->next = NULL;
			lineIndex = 0;
		} else {
			if(lineIndex >= MAX_LINE_LEN) {
				printf("max line length exceeded\n");
				exit(1);
			}

			currentLine->str[lineIndex] = *str;
			++lineIndex;
		}

		++str;
	}
	currentLine->str[lineIndex] = '\0';

	return lines;
}
