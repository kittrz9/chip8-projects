#ifndef LINES_H
#define LINES_H

#define MAX_LINE_LEN 256

typedef struct line_struct {
	char str[MAX_LINE_LEN];
	struct line_struct* next;
} line_t;

line_t* strToLines(char* str);

#endif // LINES_H
