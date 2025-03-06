#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

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
	for(uint8_t i = 0; i < depth; ++i) {
		printf("\t");
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
			printf("goto label_%04X;\n", (node->opcode & 0xFFF)-0x200);
			astWrite(node->unaryExpr.branch);
			return;
		case AST_INSTRUCTION:
			switch(node->opcode >> 12) {
				case 0:
					if((node->opcode & 0xF) == 0xE) {
						printf("return;\n");
					} else {
						printf("displayClear();\n");
					}
					break;
				case 2:
					printf("func_%04X();\n", ADDR(node->opcode)-0x200);
					break;
				case 6:
					printf("cpu.v[%i] = %i;\n", X(node->opcode), IMM(node->opcode));
					break;
				case 7:
					printf("cpu.v[%i] += %i;\n", X(node->opcode), IMM(node->opcode));
					break;
				case 8:
					switch(node->opcode & 0xF) {
						case 0:
							printf("cpu.v[%i] = cpu.v[%i];\n", X(node->opcode), Y(node->opcode));
							break;
						case 1:
							printf("cpu.v[%i] |= cpu.v[%i];\n", X(node->opcode), Y(node->opcode));
							break;
						case 2:
							printf("cpu.v[%i] &= cpu.v[%i];\n", X(node->opcode), Y(node->opcode));
							break;
						case 3:
							printf("cpu.v[%i] ^= cpu.v[%i];\n", X(node->opcode), Y(node->opcode));
							break;
						case 4:
							printf("cpu.v[%i] += cpu.v[%i];\n", X(node->opcode), Y(node->opcode));
							break;
						case 5:
							printf("cpu.v[%i] -= cpu.v[%i];\n", X(node->opcode), Y(node->opcode));
							break;
						case 6:
							printf("cpu.v[%i] >>= 1;\n", X(node->opcode));
							break;
						case 7:
							printf("cpu.v[%i] = cpu.v[%i] - cpu.v[%i];\n", X(node->opcode), Y(node->opcode), X(node->opcode));
							break;
						case 0xE:
							printf("cpu.v[%i] <<= 1;\n", X(node->opcode));
							break;
					}
					break;
				case 0xA:
					printf("cpu.i = %i;\n", IMM(node->opcode));
					break;
				case 0xC:
					printf("cpu.v[%i] = (rand()%%255) & %i;\n", X(node->opcode), IMM(node->opcode));
					break;
				case 0xD:
					printf("draw(cpu.v[%i], cpu.v[%i], %i);\n", X(node->opcode), Y(node->opcode), node->opcode&0xF);
					break;
				case 0xF:
					switch(IMM(node->opcode)) {
						case 0x29:
							printf("cpu.i = %i * 5;\n", X(node->opcode));
							break;
						case 0x33:
							printf("bcd(cpu.v[%i]);\n", X(node->opcode));
							break;
						case 0x55:
							printf("memcpy(ram+cpu.i, cpu.v, %i);\n", X(node->opcode));
							break;
						case 0x65:
							printf("memcpy(cpu.v, ram+cpu.i, %i);\n", X(node->opcode));
							break;
						default:
							printf("//!! %04X !!\n", node->opcode);
							break;
					}
					break;
				default:
					printf("//!! %04X !!\n", node->opcode);
					break;
			}
			astWrite(node->unaryExpr.branch);
			return;
		case AST_IF:
			printf("if(");
			switch(node->opcode >> 12) {
				case 3:
					printf("cpu.v[%i] == %i", X(node->opcode), IMM(node->opcode));
					break;
				case 4:
					printf("cpu.v[%i] != %i", X(node->opcode), IMM(node->opcode));
					break;
				case 5:
					printf("cpu.v[%i] != cpu.v[%i]", X(node->opcode), Y(node->opcode));
					break;
				default:
					printf("/* !! %04X !! */", node->opcode);
					break;
			}
			printf("){\n");
			++depth;
			astWrite(node->binaryExpr.left);
			--depth;
			for(uint8_t i = 0; i < depth; ++i) {
				printf("\t");
			}
			printf("}\n");
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
	printf("/*\n");
	astDebug(root);
	printf("*/\n");

	printf("\
#include <stdint.h>\n\
#include <string.h>\n\
void bcd(uint8_t);\n\
void displayClear(void);\n\
void draw(uint8_t, uint8_t, uint8_t);\n\
struct { uint8_t v[16]; uint16_t i; } cpu;\
\nuint8_t ram[0x1000];\n\
");
	size_t i = 0;
	while(funcs[i].allocated) {
		printf("void func_%04X(void);\n", funcs[i].offset);
		++i;
	}
	astWrite(root);
	
	free(buffer);

	return 0;
}
