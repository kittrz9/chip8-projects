#include "files.h"

#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"

void loadROM(char* filename) {
	FILE* f = fopen(filename, "rb");
	if(!f) {
		exit(1);
	}

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	printf("%lu bytes\n", size);

	fread(ram+0x200, size, 1, f);

	fclose(f);
}
