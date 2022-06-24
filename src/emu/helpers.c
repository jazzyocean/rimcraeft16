#include <stdint.h>
#include <string.h>
#include "emu.h"

// Converts an int to binary
void toBinary(int input, int bits, char *output) {
    for (int i = 0; i < bits; i++) {
        output[bits-i-1] = '0' + (input & 1);
        input >>= 1;
    }
    output[bits] = 0;
}

// Set word in memory
void setWord(Emulator *emu, uint16_t index, uint16_t value) {
    emu->memory[index+1] = (value >> 8) & 0xFF;
    emu->memory[index] = value & 0xFF;
}

// Fetch word from memory
uint16_t getWord(Emulator *emu, uint16_t index) {
    return emu->memory[index] | (emu->memory[index+1] << 8);
}

void push(Emulator *emu, uint16_t value) {
    emu->registers[SP] -= 2;
    emu->registers[SP] &= STACK_SIZE-1;
    setWord(emu, emu->registers[SP]+STACK, value);
}

uint16_t pop(Emulator *emu) {
    uint16_t temp = getWord(emu, emu->registers[SP]+STACK);
    emu->registers[SP] += 2;
    emu->registers[SP] &= STACK_SIZE-1;
    return temp;
}

// Return if bit index of value equals testFor
int testBit(uint16_t value, int index, int testFor) {
    return ((value >> index) & 1UL) == testFor;
}

// Sets nth bit of v to x
uint16_t setBit(uint16_t v, int n, int x) {
    return v ^= (-x ^ v) & (1UL << n);
}

void arithSetFlags(Emulator *emu, uint16_t v) {
    emu->registers[FL] = setBit(emu->registers[FL], FL_EQ, v==0);
};

// Display 0x1000 byte long bytearray, return number of lines read
int printSector(uint8_t *sector, uint16_t csector) {
    char current[114] = {0};
    char previous[114] = {0};
    int skipped = 0;
    int i;
    int lines;

    lines++;
    printf(MEMORY_TAG"\n");
    printf(MEMORY_TAG"           -0 -1 -2 -3 -4 -5 -6 -7  -8 -9 -a -b -c -d -e -f    ---0 ---2 ---4 ---6  ---8 ---a ---c ---e\n");

    for (i = 0; i < 0x1000; i += 16) {
        // Display 16 bytes
        for (int j = 0; j < 8; j++)
            sprintf(&current[3 * j], "%02x ", sector[i + j]);
        for (int j = 0; j < 8; j++)
            sprintf(&current[24 + (3 * j)], " %02x", sector[i + j + 8]);

        // Pad
        sprintf(&current[48], "    ");

        // Display 8 Little-Endian words
        for (int j = 0; j < 4; j++) {
            sprintf(&current[52 + (5 * j)], "%02x%02x ", sector[1 + i + (2 * j)], sector[i + (2 * j)]);
        }
        for (int j = 0; j < 8; j += 2)
            sprintf(&current[72 + (5 * (j / 2))], " %02x%02x", sector[1 + i + j + 8], sector[i + j + 8]);

        // Pad + bar
        sprintf(&current[92], "    |");

        // Display 16 ascii chars
        for (int j = 0; j < 16; j++)
            sprintf(&current[97 + j], "%c", sector[i + j] < 0x20 ? '.' : sector[i + j]);

        // Bar
        current[113] = '|';

        // Check if previous line is the same as this one
        int res = strcmp(current, previous);
        if (res != 0) {
            // If it's not, print it
            printf(MEMORY_TAG"%04x:%04x  %s\n", csector, i, current);
            strcpy(previous, current);
            skipped = 0;
            lines++;
        }
        else if (res == 0 && !skipped){
            // If it is, and the previous line wasnt skipped, display stars
            skipped = 1;
            printf(MEMORY_TAG"           *  *  *\n");
            lines++;
        }
        // If it is, and the previous line was skipped then like...idk man go fuck yourself
    }

    // If we skipped the last lines, show where the end bytes are
    // Not really useful for this function's specific use but im adding it anyways
    if (skipped) {
        printf(MEMORY_TAG"%04x:%04x\n", csector, i);
        lines++;
    }
    printf(MEMORY_TAG"\n");
    lines++;

    return lines;
}