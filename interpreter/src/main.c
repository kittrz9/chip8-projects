#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "files.h"
#include "cpu.h"
#include "io.h"

int main(int argc, char** argv) {
	if(argc != 2) {
		exit(1);
	}

	loadROM(argv[1]);

	screenInit();

	cpuInit();

	while(1) {
		// probably not the best for performance to poll every time an instruction is ran
		// but polling every frame lets things that run in an infinite loop not close
		SDL_Event e;
		SDL_PollEvent(&e);
		if(e.type == SDL_QUIT) { 
			break;
		}
		cpuStep();
	}

	screenUninit();

	return 0;
}
