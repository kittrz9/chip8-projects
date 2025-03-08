#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_FUNC_SIZE 256
typedef struct chip8Func {
	uint16_t offset;
	uint16_t size;
	uint16_t opcodes[MAX_FUNC_SIZE]; // probably should just use a dynamic array
	bool allocated;
} chip8Func;

#define MAX_FUNC_COUNT 32
chip8Func funcs[MAX_FUNC_COUNT];

#define MAX_ARENA_SIZE 4096*10
typedef struct {
	uint8_t buffer[MAX_ARENA_SIZE];
	uint8_t* top;
} arena;

void* arenaAlloc(arena* a, size_t size) {
	void* allocated = a->top;
	a->top += size;
	return allocated;
}

// using an AST is probably overkill for this
// but I wanted to see how using an approach like this would work in a scenario like this
// since I've only seen it done on tokenized text so far
// in a sense a chip8 rom is basically tokenized input anyways

#define AST_TYPES \
	DO(AST_IF) \
	DO(AST_JMP) \
	DO(AST_FUNCTION) \
	DO(AST_INSTRUCTION) \

typedef enum {
#define DO(type) type,
AST_TYPES
#undef DO
} astType;

char* astTypeNames[] = {
#define DO(type) [type] = #type,
AST_TYPES
#undef DO
};

typedef struct astNode {
	astType type;
	uint16_t offset;
	union {
		uint16_t opcode;
		uint16_t funcOffset;
	};
	union {
		struct {
			struct astNode* left;
			struct astNode* right;
		} binaryExpr;
		struct {
			struct astNode* branch;
		} unaryExpr;
	};
} astNode;


#define X(opcode) (opcode>>8 & 0xF)
#define Y(opcode) (opcode>>4 & 0xF)
#define IMM(opcode) (opcode& 0xFF)
#define ADDR(opcode) (opcode& 0xFFF)

astNode* parseInstruction(arena* a, uint16_t offset, uint16_t* opcodes, size_t size, uint16_t* pc, bool singular) {
	if(*pc >= size) { return NULL; }
	astNode* node = arenaAlloc(a, sizeof(astNode));
	uint16_t opcode = opcodes[*pc];
	node->opcode = opcode;
	node->offset = offset + *pc*2;
	switch(opcode >> 12) {
		case 1:
			node->type = AST_JMP;
			++*pc;
			if(!singular) {
				node->unaryExpr.branch = parseInstruction(a, offset, opcodes, size, pc, false);
			} else {
				node->unaryExpr.branch = NULL;
			}
			break;
		case 3:
		case 4:
		case 5:
		case 9:
		case 0xE:
			node->type = AST_IF;
			++*pc;
			// ideally would check if the next instruction is a jmp so it can make a larger if statement
			node->binaryExpr.left = parseInstruction(a, offset, opcodes, size, pc, true);;
			node->binaryExpr.right = parseInstruction(a, offset, opcodes, size, pc, false);
			break;
		case 0xB:
			printf("indexed jumps are not handled currently (opcode %04X)\n", opcode);
			exit(1);
		default:
			node->type = AST_INSTRUCTION;
			++*pc;
			if(!singular) {
				node->unaryExpr.branch = parseInstruction(a, offset, opcodes, size, pc, false);
			} else {
				node->unaryExpr.branch = NULL;
			}
			break;
	}
	return node;
}

astNode* parseFunction(arena* a, chip8Func* f) {
	if(f->allocated == false) { return NULL; }
	astNode* node = arenaAlloc(a, sizeof(astNode));
	node->type = AST_FUNCTION;
	node->funcOffset = f->offset;
	uint16_t pc = 0;
	node->binaryExpr.left = parseInstruction(a, f->offset, f->opcodes, f->size, &pc, false);
	++f;
	node->binaryExpr.right = parseFunction(a, f);
	return node;
}




// really need a better name
bool checkIfAlreadyGrabbed(uint16_t offset) {
	uint8_t i = 0;
	while(funcs[i].allocated == true) {
		if(offset >= funcs[i].offset && offset < funcs[i].offset + funcs[i].size) { return true; }
		++i;
	}
	return false;
}

void grabFunc(uint8_t* rom, size_t romSize, uint16_t offset) {
	if(checkIfAlreadyGrabbed(offset)) { return; }
	uint8_t i = 0;
	while(funcs[i].allocated == true) {++i;}
	chip8Func* f = &funcs[i];
	f->allocated = true;
	f->offset = offset;
	uint16_t pc = offset;
	while(pc < romSize) {
		uint16_t currentOpcode = rom[pc]<<8 | rom[pc+1];
		uint16_t lastOpcode = 0;
		if(pc != 0) { // to make sure it doesn't read out of bounds
			lastOpcode = rom[pc-2]<<8 | rom[pc-1];
		}
		f->opcodes[f->size] = currentOpcode;
		//printf("%i - %i - %04X\n", i, f->size,  currentOpcode);
		f->size += 1;
		switch(currentOpcode >> 12) {
			case 1:
			case 2:
				//printf("%04X - %04X\n", pc+0x200, ADDR-0x200);
				grabFunc(rom, romSize, ADDR(currentOpcode)-0x200);
				break;
		}
		// if the current opcode is a jump and cant be skipped, end chunk
		switch(lastOpcode >> 12) {
			case 3:
			case 4:
			case 5:
			case 9:
				break;
			default:
				if(currentOpcode >> 12 == 1 || currentOpcode >> 12 == 0xB || currentOpcode == 0x00EE) {
					return;
				}
				break;
		}
		pc += 2;
	}
}

void astDebug(astNode* node) {
	static uint8_t depth = 1;
	if(node == NULL) { return; }
	printf("%*c%s ", depth, '>', astTypeNames[node->type]);
	++depth;
	switch(node->type) {
		case AST_FUNCTION:
		case AST_IF:
			printf("%04X\n", node->funcOffset);
			astDebug(node->binaryExpr.left);
			astDebug(node->binaryExpr.right);
			break;
		case AST_INSTRUCTION:
		case AST_JMP:
			printf("%04X\n", node->opcode);
			astDebug(node->unaryExpr.branch);
			break;
		default:
			printf("\nunhandled ast node type %s\n", astTypeNames[node->type]);
			exit(1);
			break;
	}
	--depth;
}

bool labelsUsed[0x1000];

void astFindUsedLabels(astNode* node) {
	if(node == NULL) { return; }
	switch(node->type) {
		case AST_FUNCTION:
			astFindUsedLabels(node->binaryExpr.left);
			astFindUsedLabels(node->binaryExpr.right);
			return;
		case AST_JMP:
			labelsUsed[(node->opcode & 0xFFF)-0x200] = true;
			astFindUsedLabels(node->unaryExpr.branch);
			return;
		case AST_INSTRUCTION:
			astFindUsedLabels(node->unaryExpr.branch);
			return;
		case AST_IF:
			astFindUsedLabels(node->binaryExpr.left);
			astFindUsedLabels(node->binaryExpr.right);
			return;
		default:
			printf("unhandled ast node type %s in astFindUsedLabels\n", astTypeNames[node->type]);
			exit(1);
	}
	printf("unreachable\n");
	exit(1);
}

void astWrite(astNode* node) {
	if(node == NULL) { return; }
	if(labelsUsed[node->offset]) {
		printf("label_%04X:\n", node->offset);
	}
	static uint8_t depth = 0;
	char tabs[4] = "";
	for(uint8_t i = 0; i < depth; ++i) {
		strncat(tabs, "\t", 1);
	}
	switch(node->type) {
		case AST_FUNCTION:
			// a lot of the roms I've seen have a jmp at the entry point
			// this solves dealing with that trivial jmp
			if(node->binaryExpr.left->type == AST_JMP) {
				printf("#define func_%04X func_%04X\n", node->funcOffset, (node->binaryExpr.left->opcode&0xFFF)-0x200);
			} else {
				printf("void func_%04X(void) {\n", node->funcOffset);
				++depth;
				astWrite(node->binaryExpr.left);
				--depth;
				printf("}\n");
			}
			astWrite(node->binaryExpr.right);
			return;

		case AST_JMP:
			//printf("//!! JMP %04X -> %04X !!\n", node->offset, (node->opcode & 0xFFF) - 0x200);
			// could probably change this to a do {...} while loop
			printf("%sgoto label_%04X;\n", tabs, (node->opcode & 0xFFF)-0x200);
			astWrite(node->unaryExpr.branch);
			return;
		case AST_INSTRUCTION:
			switch(node->opcode >> 12) {
				case 0:
					if((node->opcode & 0xF) == 0xE) {
						printf("%sreturn;\n", tabs);
					} else {
						printf("%smemset(frameBuffer, 0, sizeof(frameBuffer));\n", tabs);
					}
					break;
				case 2:
					printf("%sfunc_%04X();\n", tabs, ADDR(node->opcode)-0x200);
					break;
				case 6:
					printf("%scpu.v[%i] = %i;\n", tabs, X(node->opcode), IMM(node->opcode));
					break;
				case 7:
					printf("%scpu.v[%i] += %i;\n", tabs, X(node->opcode), IMM(node->opcode));
					break;
				case 8:
					switch(node->opcode & 0xF) {
						case 0:
							printf("%scpu.v[%i] = cpu.v[%i];\n", tabs, X(node->opcode), Y(node->opcode));
							break;
						case 1:
							printf("%scpu.v[%i] |= cpu.v[%i];\n", tabs, X(node->opcode), Y(node->opcode));
							break;
						case 2:
							printf("%scpu.v[%i] &= cpu.v[%i];\n", tabs, X(node->opcode), Y(node->opcode));
							break;
						case 3:
							printf("%scpu.v[%i] ^= cpu.v[%i];\n", tabs, X(node->opcode), Y(node->opcode));
							break;
						case 4:
							printf("%scpu.v[%i] += cpu.v[%i];\n", tabs, X(node->opcode), Y(node->opcode));
							printf("%scpu.v[15] = ((uint16_t)cpu.v[%i] + (uint16_t)cpu.v[%i]) > 255;\n", tabs, X(node->opcode), Y(node->opcode));
							break;
						case 5:
							printf("%scpu.v[%i] -= cpu.v[%i];\n", tabs, X(node->opcode), Y(node->opcode));
							printf("%scpu.v[15] = cpu.v[%i] > cpu.v[%i];\n", tabs, X(node->opcode), Y(node->opcode));
							break;
						case 6:
							printf("%scpu.v[%i] >>= 1;\n", tabs, X(node->opcode));
							printf("%scpu.v[15] = cpu.v[%i] & 1;\n", tabs, X(node->opcode));
							break;
						case 7:
							printf("%scpu.v[%i] = cpu.v[%i] - cpu.v[%i];\n", tabs, X(node->opcode), Y(node->opcode), X(node->opcode));
							printf("%scpu.v[15] = cpu.v[%i] > cpu.v[%i];\n", tabs, Y(node->opcode), X(node->opcode));
							break;
						case 0xE:
							printf("%scpu.v[%i] <<= 1;\n", tabs, X(node->opcode));
							printf("%scpu.v[15] = cpu.v[%i] >> 7;\n", tabs, X(node->opcode));
							break;
					}
					break;
				case 0xA:
					// ideally would see how this address is used and then make that part of the rom its own thing
					// instead of just having all of ram as an array
					printf("%scpu.i = %i;\n", tabs, ADDR(node->opcode));
					break;
				case 0xC:
					printf("%scpu.v[%i] = (rand()%%255) & %i;\n", tabs, X(node->opcode), IMM(node->opcode));
					break;
				case 0xD:
					printf("%sdraw(cpu.v[%i], cpu.v[%i], %i);\n", tabs, X(node->opcode), Y(node->opcode), node->opcode&0xF);
					break;
				case 0xF:
					switch(IMM(node->opcode)) {
						case 0x1E:
							printf("%scpu.i += cpu.v[%i];\n", tabs, X(node->opcode));
							break;
						case 0x29:
							printf("%scpu.i = cpu.v[%i] * 5;\n", tabs, X(node->opcode));
							break;
						case 0x33:
							printf("%sbcd(cpu.v[%i]);\n", tabs, X(node->opcode));
							break;
						case 0x55:
							printf("%smemcpy(ram+cpu.i, cpu.v, %i);\n", tabs, X(node->opcode)+1);
							printf("%scpu.i += %i;\n", tabs, X(node->opcode+1));
							break;
						case 0x65:
							printf("%smemcpy(cpu.v, ram+cpu.i, %i);\n", tabs, X(node->opcode)+1);
							printf("%scpu.i += %i;\n", tabs, X(node->opcode+1));
							break;
						default:
							printf("%s//!! %04X !!\n", tabs, node->opcode);
							break;
					}
					break;
				default:
					printf("%s//!! %04X !!\n", tabs, node->opcode);
					break;
			}
			astWrite(node->unaryExpr.branch);
			return;
		case AST_IF:
			printf("%sif(", tabs);
			switch(node->opcode >> 12) {
				case 3:
					printf("cpu.v[%i] != %i", X(node->opcode), IMM(node->opcode));
					break;
				case 4:
					printf("cpu.v[%i] == %i", X(node->opcode), IMM(node->opcode));
					break;
				case 5:
					printf("cpu.v[%i] != cpu.v[%i]", X(node->opcode), Y(node->opcode));
					break;
				case 9:
					printf("cpu.v[%i] == cpu.v[%i]", X(node->opcode), Y(node->opcode));
					break;
				default:
					printf("/* !! %04X !! */", node->opcode);
					break;
			}
			printf("){\n");
			++depth;
			astWrite(node->binaryExpr.left);
			--depth;
			printf("%s}\n", tabs);
			astWrite(node->binaryExpr.right);
			return;
		default:
			printf("\nunhandled ast node type %s\n", astTypeNames[node->type]);
			exit(1);
	}
	printf("unreachable\n");
	exit(1);
}

int main(int argc, char** argv) {
	if(argc < 2) { return 1; }

	FILE* file = fopen(argv[1], "rb");
	if(file == NULL) { return 1; }
	fseek(file, 0, SEEK_END);
	size_t fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	//printf("%zu\n", fileSize);

	uint8_t* buffer = malloc(fileSize);
	fread(buffer, 1, fileSize, file);

	/*for(size_t i = 0; i < fileSize; ++i) {
		printf("%02X,", buffer[i]);
	}
	printf("\n");*/

	grabFunc(buffer, fileSize, 0);

	arena a;
	a.top = a.buffer;
	astNode* root = parseFunction(&a, &funcs[0]);
	astFindUsedLabels(root);
	#if 0
	printf("/*\n");
	astDebug(root);
	printf("*/\n");
	#endif

	printf("\
#include \"raylib.h\"\n\
#include <stdint.h>\n\
#include <string.h>\n\
#include <stdio.h>\n\
#include <stdlib.h>\n\
struct { uint8_t v[16]; uint16_t i; } cpu;\n\
uint8_t ram[0x1000];\n\
uint8_t frameBuffer[64*32];\n\
uint8_t font[] = {\n\
	0xF0, 0x90, 0x90, 0x90, 0xF0,\n\
	0x20, 0x60, 0x20, 0x20, 0x70,\n\
	0xF0, 0x10, 0xF0, 0x80, 0xF0,\n\
	0xF0, 0x10, 0xF0, 0x10, 0xF0,\n\
	0x90, 0x90, 0xF0, 0x10, 0x10,\n\
	0xF0, 0x80, 0xF0, 0x10, 0xF0,\n\
	0xF0, 0x80, 0xF0, 0x90, 0xF0,\n\
	0xF0, 0x10, 0x20, 0x40, 0x40,\n\
	0xF0, 0x90, 0xF0, 0x90, 0xF0,\n\
	0xF0, 0x90, 0xF0, 0x10, 0xF0,\n\
	0xF0, 0x90, 0xF0, 0x90, 0x90,\n\
	0xE0, 0x90, 0xE0, 0x90, 0xE0,\n\
	0xF0, 0x80, 0x80, 0x80, 0xF0,\n\
	0xE0, 0x90, 0x90, 0x90, 0xE0,\n\
	0xF0, 0x80, 0xF0, 0x80, 0xF0,\n\
	0xF0, 0x80, 0xF0, 0x80, 0x80\n\
};\n\
void bcd(uint8_t value) {\n\
	for(uint8_t i = 0; i < 3; ++i) {\n\
		uint8_t digit = value %% 10;\n\
		ram[cpu.i + (2-i)] = digit;\n\
		value = (value - digit)/10;\n\
	}\n\
}\n\
void draw(uint8_t x, uint8_t y, uint8_t n) {\n\
	x = x %% 64;\n\
	y = y %% 64;\n\
	uint8_t* source = &ram[cpu.i];\n\
	cpu.v[15] = 0;\n\
	for(uint8_t i = 0; i < n; ++i) {\n\
		for(uint8_t j = 0; j < 8; ++j) {\n\
			uint8_t* target = &frameBuffer[x + y*64];\n\
			uint8_t original = *target;\n\
			uint8_t bit = (*source >> (7-j)) & 1;\n\
			*target = (0xFF * bit)^original;\n\
			if(*target == 0 && original == 0xFF) {\n\
				cpu.v[15] = 1;\n\
			}\n\
			x = (x+1)%%64;\n\
		}\n\
		++source;\n\
		y = (y+1)%%32;\n\
		x -= 8;\n\
	}\n\
	if(WindowShouldClose()) { CloseWindow(); exit(1); }\n\
	BeginDrawing();\n\
		ClearBackground(BLACK);\n\
		for(uint8_t j = 0; j < 32; ++j) {\n\
			for(uint8_t i = 0; i < 64; ++i) {\n\
				if(frameBuffer[i + j*64] == 0xFF) {\n\
					DrawRectangle(i*10, j*10, 10, 10, WHITE);\n\
				}\n\
			}\n\
		}\n\
	EndDrawing();\n\
"
/*
	printf(\"\\x1B[H\");\n\
	for(uint8_t j = 0; j < 32; ++j) {\n\
		for(uint8_t i = 0; i < 64; ++i) {\n\
			if(frameBuffer[i + j*64] == 0xFF) {\n\
				printf(\"#\");\n\
			} else {\n\
				printf(\" \");\n\
			}\n\
		}\n\
		printf(\"\\n\");\n\
	}\n\
*/
"\
}\n\
");
	// including all of the rom until I can make it figure out what parts of memory are read from by the program
	// will eventually copy this into the ram array once I start working on the runtime stuff
	// could also probably just initialize the ram array with these values along with the font, but that would require including tons of zeros
	printf("uint8_t rom[%zu] = {", fileSize);
	for(size_t i = 0; i < fileSize; ++i) {
		printf("0x%02X,", buffer[i]);
	}
	printf("};\n");
	size_t i = 0;
	while(funcs[i].allocated) {
		printf("void func_%04X(void);\n", funcs[i].offset);
		++i;
	}
	astWrite(root);

	printf("\
int main(int argc, char** argv) {\n\
	InitWindow(640, 320, \"asdfasdf\");\n\
	memcpy(ram+0x200, rom, %zu);\n\
	memcpy(ram, font, sizeof(font));\n\
	func_0000();\n\
}\n\
", fileSize);
	
	free(buffer);

	return 0;
}
