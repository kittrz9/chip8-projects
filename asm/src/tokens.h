#ifndef TOKENS_H
#define TOKENS_H

#include <stdint.h>

#define MAX_TOKEN_LEN 32

typedef enum {
	TOKEN_LABEL,
	TOKEN_LABEL_ADDR, // bad name, this is when a label is used in an instrction like "jmp loop"
	TOKEN_INSTRUCTION,
	TOKEN_REGISTER,
	TOKEN_NUMBER,
} token_type;

typedef struct token_struct {
	token_type type;
	char str[MAX_TOKEN_LEN];
	struct token_struct* next;
} token_t;

token_t* tokenize(char* str);
token_t* tokenNext(token_t* token);


uint8_t getTokenReg(token_t* t);
uint8_t getTokenNum(token_t* t);
uint16_t getTokenLabelAddr(token_t* t);

#endif // TOKENS_H
