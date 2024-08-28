#include "io.h"

#include <SDL2/SDL.h>

uint32_t* fbPixels = NULL;

SDL_Surface* frameBuffer;
SDL_Surface* windowSurface;
SDL_Window* w;

void screenClear(void) {
	for(uint16_t i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; ++i) {
		fbPixels[i] = 0xFF;
	}
	return;
}

void screenUpdate(void) {
	SDL_BlitScaled(frameBuffer, &(SDL_Rect){0,0,FRAME_WIDTH,FRAME_HEIGHT}, windowSurface, &(SDL_Rect){0,0,FRAME_WIDTH*FRAME_SCALE,FRAME_HEIGHT*FRAME_SCALE});
	SDL_UpdateWindowSurface(w);
}

void screenInit(void) {
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		exit(1);
	}

	w = SDL_CreateWindow("asdfasdfsadf", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, FRAME_WIDTH * FRAME_SCALE, FRAME_HEIGHT * FRAME_SCALE, 0); 
	frameBuffer = SDL_CreateRGBSurface(0, FRAME_WIDTH, FRAME_HEIGHT, 32, 0xFF000000, 0xFF0000, 0xFF00, 0xFF);
	windowSurface = SDL_GetWindowSurface(w);

	fbPixels = (uint32_t*)frameBuffer->pixels;

	screenClear();

	screenUpdate();
}

void screenUninit(void) {
	SDL_FreeSurface(frameBuffer);
	SDL_DestroyWindowSurface(w);
	SDL_DestroyWindow(w);
}
