#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include "consts.h"
#include "helpers.h"
#include "emu.h"

int main(int argc, char **argv) {
    int flags = 0b00000000;
    // -v: 0b00000001  verbose
    // -s: 0b00000010  spacebar for every instuction
    // -S: 0b00000100  spacebar every breakpoint

    if (argc == 1) {
        printf(ERROR_TAG"Input file required");
        return -1;
    }

    if (argc > 2) {
        for (int i=2; i<argc; i++) {
            if (argv[i][0] == '-') {
                switch (argv[i][1]) {
                    case 'v': {
                        flags |= 0b00000001;
                        printf(INFO_TAG"Verbose mode enabled\n");
                        break;
                    } case 'e': {
                        printf(INFO_TAG"Line-by-line mode enabled\n");
                        flags |= 0b00000010;
                        break;
                    } case 'b': {
                        printf(INFO_TAG"Breakpoint mode enabled\n");
                        flags |= 0b00000100;
                        break;
                    }
                }
            }
        }
    }

    Emulator emu = {.instructionCounter = 0, .tickCounter = 0, .clargs = flags};
    if (initEmulator(&emu, argv[1], flags) == -1) {
        printf(ERROR_TAG"Error initializing emulator state\n");
        return -2;
    }
    
    printf(INFO_TAG"Initializing threads.\n");
    if (initThreads(&emu) != 0) {
        printf(ERROR_TAG"Error initializing threads\n");
        return -3;
    }

    printSector(emu.memory, 0);
    printf(INFO_TAG"Emulator started.\n");
    emulationLoop(&emu);
    printf(INFO_TAG"Emulator Ended.\n");
    printSector(&emu.memory[VRAM], 0xa);
    
    double secondsElapsed = ((double)(clock() - emu.startTime))/CLOCKS_PER_SEC;
    printf(INFO_TAG"Executed %lli instructions (%lli ticks) in %Lfs (%f instuctions/sec, %f ticks/sec)\n", emu.instructionCounter, emu.tickCounter, secondsElapsed, ((double)emu.instructionCounter)/secondsElapsed, ((double)emu.tickCounter)/secondsElapsed);
    
    char tempString[17];
    toBinary(emu.registers[FL], 16, tempString);

    printf(INFO_TAG"G0 %04x!   G1 %04x!\n\
          G2 %04x!   G3 %04x!\n\
          G4 %04x!   G5 %04x!\n\
          IX %04x!   PC %04x!\n\
          SP %04x    FL %s   \n\
          CL %04x",
          emu.registers[G0], emu.registers[G1],
          emu.registers[G2], emu.registers[G3],
          emu.registers[G4], emu.registers[G5],
          emu.registers[IX], emu.registers[PC],
          emu.registers[SP], tempString,
          emu.registers[CL]);

    free(emu.rom);

    return 0;
}