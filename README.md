# a few chip-8 projects

currently just a chip-8 emulator (with both an interpreter and JIT compiler mode), disassembler, and assembler<br><br>
all currently unfinished<br><br>
I hope to eventually write a static recompiler, but idk if I'll ever get around to that<br><br>
the disassembler's output has names for each opcode given by me because I don't think there's any agreed upon names and I currently don't care enough to check<br><br>
the interpreter mode of the emulator runs but has very clear bugs with at least one of the math opcodes<br><br>
the JIT compiler mode is very unfinished, with only a few opcodes implemented and no caching<br><br>
to set the mode of the emulator, change the `MODE` variable in `emu/build.sh`<br><br>
to compile any of the projects, `cd` into the project's directory and run `./build.sh`<br><br>
