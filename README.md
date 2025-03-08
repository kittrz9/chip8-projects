# a few chip-8 projects

currently just a chip-8 emulator (with both an interpreter and JIT compiler mode), disassembler, assembler, and static recompiler<br><br>
all currently unfinished<br><br>
<br>
the disassembler's output has names for each opcode given by me because I don't think there's any agreed upon names and I currently don't care enough to check<br><br>
<br>
the JIT compiler mode of the emulator is very unfinished, with only a few opcodes implemented and no caching<br><br>
to set the mode of the emulator, change the `MODE` variable in `emu/build.sh`<br><br>
<br>
the static recompiler seems to work (though I've only tested it with one rom), but it doesn't have the input or timer related functions implemented yet.<br>
the recompiled programs use raylib for graphics, there is an included script to compile them in the recomp folder<br>
<br>
to compile any of the projects, `cd` into the project's directory and run `./build.sh`<br><br>
