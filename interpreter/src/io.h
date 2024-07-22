#ifndef IO_H
#define IO_H

// currently only display stuff but will handle input eventually

#include <stdint.h>

#define FRAME_WIDTH 64
#define FRAME_HEIGHT 32

#define FRAME_SCALE 10

// RGBA
extern uint32_t* fbPixels;

void screenClear(void);
void screenUpdate(void);
void screenInit(void);
void screenUninit(void);

#endif // IO_H
