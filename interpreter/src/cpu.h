#ifndef CPU_H
#define CPU_H

#include <stdint.h>

extern uint8_t ram[0x1000];

void cpuStep(void);
void cpuInit(void);

#endif // CPU_H
