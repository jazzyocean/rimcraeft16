#ifndef CONSTS_H
#define CONSTS_H

#define G0 0   //~General use
#define G1 1   //~General use
#define G2 2   //~General use
#define G3 3   //~General use
#define G4 4   //~General use
#define G5 5   //~General use
#define IX 6   //~Index
#define PC 7   //~Program counter
#define SP 8   // Stack pointer
#define FL 9   // Flags
#define CL 10  // Clock

#define VRAM  0xA000 // -> 0xFFFF
#define PHWIO 0x9800 // -> 0x9FFF
// :
#define WRAM_BANK 0x9B02
#define MEM_MODE  0x9B00
#define PININ2    0x9A06
#define PININ1    0x9A04
#define PINOUT2   0x9A02
#define PINOUT1   0x9A00
#define CONTROL4  0x98FE
#define CONTROL3  0x98FC
#define CONTROL2  0x98FA
#define CONTROL1  0x98F8
#define AUDIO3    0x98F6
#define AUDIO2    0x98F4
#define ADUIO1    0x98F2
#define RNGSEED   0x98F0
#define RNGOUT    0x98EE
//  ;
#define STACK 0x9000 // -> 0x97FF
#define WRAM4 0x8000 // -> 0x8FFF
#define WRAM3 0x7000 // -> 0x7FFF
#define WRAM2 0x6000 // -> 0x6FFF
#define WRAM1 0x5000 // -> 0x5FFF
#define INTC  0x4400 // -> 0x4FFF
#define IVT   0x4000 // -> 0x43FF
#define ROM_3 0x3000 // -> 0x3FFF
#define ROM_2 0x2000 // -> 0x2FFF
#define ROM_1 0x1000 // -> 0x1FFF
#define ROM_0 0x0000 // -> 0x0FFF
//  :
#define S0_NUM_SECTORS 0x0FFE
#define S0_ENTRY_POINT 0x0FFC
#define S0_ROM_FLAGS   0x0FFA
//  ;

#define STACK_SIZE 0x800

#define FL_OF 0  // 000 OFf
#define FL_EQ 1  // 001 EQual/zero
#define FL_LT 2  // 010 Lesser Than
#define FL_GT 3  // 011 Greater Than
#define FL_CR 4  // 100 CaRry
#define FL_IN 5  // 101 INterrupt inhibit
#define FL_IR 6  // 110 INterrupt inhibit
#define FL_INT 7  // 111 INTerrupt active


#define DEBUG_TAG  "[VERBOSE] "
#define INFO_TAG   "[   INFO] "
#define ERROR_TAG  "[  ERROR] "
#define MEMORY_TAG "[ MEMORY]"
#define TAG_LENGTH 9

#endif