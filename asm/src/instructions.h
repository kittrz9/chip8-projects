#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdint.h>

#include "tokens.h"

uint8_t validInstruction(char* str);
uint16_t processInstruction(token_t** currentToken, uint16_t pc);


#endif // INSTRUCTIONS_H
