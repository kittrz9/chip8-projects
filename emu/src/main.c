#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "files.h"
#include "cpu.h"
#include "io.h"

#ifdef CPU_INTERP
#ifdef CPU_JIT
#error both CPU_INTERP and CPU_JIT defined
#endif
#endif

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
		//cpuStep();
		#ifdef CPU_INTERP
		cpuInterpStep();
		#endif
		#ifdef CPU_JIT
		cpuJitRun();
		#endif
	}

	screenUninit();

	return 0;
}
