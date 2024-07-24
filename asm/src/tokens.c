#include "tokens.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

// will eventually move this array to a file specifically for instructions probably
char* instructionNames[] = {
	"clear",
	"ret",
	"jmp",
	"call",
	"skip_eq",
	"skip_ne",
	"ldv",
	"add",
	"or",
	"and",
	"xor",
	"sub",
	"sub2",
	"shift_r",
	"shift_l",
	"ldi",
	"rand",
	"draw",
	"skip_key_down",
	"skip_key_up",
	"get_delay",
	"set_delay",
	"get_sound",
	"set_sound",
	"add_i",
	"get_sprite",
	"bcd",
	"dump_reg",
	"load_reg",
};

uint8_t endsWith(char* str, char c) {
	while(*str != '\0') {
		++str;
	}
	--str;
	return *str == c;
}

token_t* tokenCreate(token_type t, char* str) {
	token_t* newToken = malloc(sizeof(token_t));
	newToken->type = t;
	newToken->next = NULL;
	strncpy(newToken->str, str, MAX_TOKEN_LEN);
	return newToken;
}

token_t* tokenize(char* str) {
	char* tokenStr = strtok(str, " \n\t");
	token_t* tokens = NULL;
	token_t* lastToken = NULL;
	while(tokenStr != NULL) {
		token_t* newToken = NULL;
		if(endsWith(tokenStr, ':')) {
			printf("label!!! %s\n", tokenStr);
			newToken = tokenCreate(TOKEN_LABEL, tokenStr);
		} else if(tokenStr[0] == 'v') {
			printf("register!!!! %s\n", tokenStr);
			newToken = tokenCreate(TOKEN_REGISTER, tokenStr);
		} else if(tokenStr[0] == '#') {
			printf("number!!!! %s\n", tokenStr);
			newToken = tokenCreate(TOKEN_NUMBER, tokenStr);
		} else if(tokenStr[0] == '@') {
			printf("label address!!!! %s\n", tokenStr);
			newToken = tokenCreate(TOKEN_LABEL_ADDR, tokenStr);
		} else {
			for(uint8_t i = 0; i < sizeof(instructionNames)/sizeof(instructionNames[0]); ++i) {
				if(strcmp(tokenStr, instructionNames[i]) == 0) {
					printf("instruction!!!! %s\n", tokenStr);
					newToken = tokenCreate(TOKEN_INSTRUCTION, tokenStr);
				}
			}
			if(newToken == NULL) {
				printf("unrecognized token %s %02X\n", tokenStr, *tokenStr);
				exit(1);
			}
		}
		if(tokens == NULL) {
			tokens = newToken;
		} else {
			lastToken->next = newToken;
		}
		lastToken = newToken;
		tokenStr = strtok(NULL, " \n\t,");
	}
	return tokens;
}

token_t* tokenNext(token_t* token) {
	token_t* next = token->next;
	free(token);
	return next;
}

uint8_t getTokenReg(token_t* t) {
	if(t->type != TOKEN_REGISTER) {
		printf("expected register, got \"%s\"\n", t->str);
		exit(1);
	}
	return hexStrToInt(t->str+1, 1); // skip the v at the start
}
uint8_t getTokenNum(token_t* t) {
	if(t->type != TOKEN_NUMBER) {
		printf("expected number, got \"%s\"\n", t->str);
		exit(1);
	}
	return hexStrToInt(t->str+1, 3); // 3 maximum
}

uint16_t getTokenLabelAddr(token_t* t) {
	if(t->type != TOKEN_LABEL_ADDR) {
		printf("expected label, got \"%s\"\n", t->str);
		exit(1);
	}
	return findLabel(t->str+1)->address; // 3 maximum
}
