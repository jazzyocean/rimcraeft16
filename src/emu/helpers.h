#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>
#include "emu.h"

void toBinary(int input, int bits, char *output);
void setWord(Emulator *emu, uint16_t index, uint16_t value);
uint16_t getWord(Emulator *emu, uint16_t index);
void push(Emulator *emu, uint16_t value);
uint16_t pop(Emulator *emu);
int testBit(uint16_t value, int index, int testFor);
uint16_t setBit(uint16_t v, int n, int x);
void arithSetFlags(Emulator *emu, uint16_t v);
int printSector(uint8_t *sector, uint16_t csector);

#endif