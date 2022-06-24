#ifndef EMU_H
#define EMU_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "consts.h"

struct Emulator {
    uint8_t memory[65536];
    uint16_t registers[12];
    uint64_t tickCounter;
    uint64_t instructionCounter;
    clock_t startTime;
    uint64_t clargs;
    uint8_t *rom;
    uint8_t intID;
    uint16_t prevRomSel[4];
    int breakpoint;
};

typedef struct Emulator Emulator;

int initThreads(Emulator *emu);
int loadSector(Emulator *emu, uint16_t sector, int slot);
int initEmulator(Emulator *emu, char *file, int flags);
void emulationLoop(Emulator *emu);

#endif