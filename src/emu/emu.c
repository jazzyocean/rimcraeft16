#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include "include/raylib.h"
#include "consts.h"
#include "helpers.h"
#include "emu.h"
#include "fileio.h"

void *t_Clock(void *args) {
    // Threaded function that increments the clock
    Emulator *emu = (Emulator*)args;

    clock_t prev = clock();
    while (testBit(emu->registers[FL], FL_OF, 0)) {
        if (((double)(clock() - prev))/CLOCKS_PER_SEC >= 0.02) {
            prev = clock();
            emu->registers[CL]++;
        }
    }
    
    return 0;
}

void *t_Video(void *args) {
    // Threaded function that displays video
    Emulator *emu = (Emulator*)args;
    
    const int screenWidth = 512;
    const int screenHeight = 496;
    const int charsize = 16;
    const int countW = screenWidth/charsize;
    const int countH = screenHeight/charsize;

    SetTraceLogLevel(LOG_NONE);
    InitWindow(screenWidth, screenHeight, "RIMCRÆFT VIDEO");
    SetTargetFPS(60);

    Font font = LoadFontEx("./src/emu/resources/Px437_IBM_BIOS.ttf", charsize, 0, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);

    clock_t prev = clock();
    while (testBit(emu->registers[FL], FL_OF, 0) && !WindowShouldClose()) {
        if (((double)(clock() - prev))/CLOCKS_PER_SEC >= 0.01) {
            BeginDrawing();
                ClearBackground((Color){0, 0, 0, 1});
                for (int y = 0; y < countH; y++) {
                    for (int x = 0; x < countW; x++) {
                        char str[2];
                        str[0] = emu->memory[VRAM+x+(y*countW)];
                        str[1] = 0;
                        DrawTextEx(font, str, (Vector2){x*charsize, y*charsize}, charsize, 0, WHITE);
                    }
                }
            EndDrawing();
        }
    }
    
    return 0;
}

int initThreads(Emulator *emu) {
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, *t_Clock, (void*)emu);
    printf(INFO_TAG"Created clock thread with thread id %u\n", thread_id);

    pthread_create(&thread_id, NULL, *t_Video, (void*)emu);
    printf(INFO_TAG"Created video thread with thread id %u\n", thread_id);

    return 0;
}

// This seems important enough that I should put it here
int loadSector(Emulator *emu, uint16_t sector, int slot) {
    for (uint16_t i = 0; i < 0x1000; i++) {
        emu->memory[ROM_0 + (slot*0x1000) + i] = emu->rom[((uint32_t)sector * 0x1000) + i];
    }
    
    return 0;
}

int initEmulator(Emulator *emu, char *file, int flags) {
    printf(INFO_TAG"Initializing Emulator\n");
    if ((flags & 0b00001000) >> 3) {
        // use platform specific function
    } else {
        emu->rom = copy(file);
        if (emu->rom == NULL) return -1;
    }

    clock_t start = clock();
    loadSector(emu, 0, 0);
    double secondsElapsed = (double)(start-clock())/CLOCKS_PER_SEC;

    printf(INFO_TAG"Loaded sector 0 in %dms\n", secondsElapsed*1000);
    
    emu->registers[PC] = emu->memory[ROM_0+S0_ENTRY_POINT] | emu->memory[ROM_0+S0_ENTRY_POINT+1];
    printf(INFO_TAG"   : ROM Flags: 0x%04x\n", emu->memory[ROM_0+S0_ROM_FLAGS] | emu->memory[ROM_0+S0_ROM_FLAGS+1]);
    printf(INFO_TAG"   : Num Sectors in ROM: 0x%04x\n", emu->memory[ROM_0+S0_NUM_SECTORS] | emu->memory[ROM_0+S0_NUM_SECTORS+1]);
    printf(INFO_TAG"   : Entry Point: 0x%04x\n", emu->memory[ROM_0+S0_ENTRY_POINT] | emu->memory[ROM_0+S0_ENTRY_POINT+1]);

    emu->startTime = clock();
}

void debugImmn(Emulator *emu, char *opc, uint16_t immn, int dst) {
    if (emu->clargs & 1) printf("%04x  %s 0x%04x -> r%x\n", immn, opc, immn, dst);
}
void debugi(Emulator *emu, char *opc, uint16_t immn) {
    if (emu->clargs & 1) printf("%04x  %s 0x%04x\n", immn, opc, immn);
}
void debug(Emulator *emu, char *opc, int src, int dst) {
    if (emu->clargs & 1) printf("      %s r%x -> r%x\n", opc, src, dst);
}
void debugr(Emulator *emu, char *opc, int dst) {
    if (emu->clargs & 1) printf("      %s r%x\n", opc, dst);
}
void debugs(Emulator *emu, char *opc, uint16_t immn, int src) {
    if (emu->clargs & 1) printf("%04x  %s r%x -> 0x%04x\n", immn, opc, src, immn);
}


// Mainloop of Emulator emulation
void emulationLoop(Emulator *emu) {
    // While flag 0 is 0
    while (testBit(emu->registers[FL], FL_OF, 0)) {
        
        uint16_t inst = getWord(emu, emu->registers[PC]);

        // 0 0 0 0 0 0 0 0 | 0 0 0 0 0 0 0 0
        // ----------- --- | --- ----- -----
        // op          cn    cn  dst    src

        int src = inst & 0b111;
        int dst = (inst >> 3) & 0b111;
        int cn = (inst >> 6) & 0b1111;
        int op = inst >> 10;
        uint16_t temp;

        emu->instructionCounter++;
        emu->tickCounter++;
        emu->registers[PC] += 2;

        // Displays info about th   
        if (emu->clargs&1) printf(DEBUG_TAG"@%04x || (src:%x dst:%x cond:%x inst:%02x)  %04x ", emu->registers[PC]-2, src, dst, cn, op, inst);

        uint16_t immn;
        if (testBit(emu->registers[FL], cn >> 1, cn & 1)) {
            //  
            //  ARITHMATIC INSTRUCTIONS
            //  
            if (op <= 0x7 || (op >= 0xa && op <= 0xd) || op == 0x10 || op == 0x12 || (op >= 0x14 && op <= 0x18) || (op >= 0x1a && op <= 0x22) || (op >= 0x26 && op <= 0x2c)) {
                if (op == 0x00 || op == 0x02 || op == 0x04 || op == 0x06 || op == 0x0a || op == 0x10 || op == 0x12 || op == 0x14 || op == 0x16 || op == 0x1b || op == 0x1d || op == 0x20 || op == 0x22 || op == 0x27 || op == 0x29) {
                    immn = getWord(emu, emu->registers[PC]);
                    emu->registers[PC] += 2;
                    emu->tickCounter += 2;
                }

                switch (op) {
                    case 0x0:
                        emu->registers[dst] += immn;
                        debugImmn(emu, "add", immn, dst);
                        break;
                    case 0x1:
                        emu->registers[dst] += emu->registers[src];
                        debug(emu, "add", src, dst);
                        break;
                    case 0x2:
                        emu->registers[dst] -= immn;
                        debugImmn(emu, "sub", immn, dst);
                        break;
                    case 0x3:
                        emu->registers[dst] -= emu->registers[src];
                        debug(emu, "sub", src, dst);
                        break;
                    case 0x4:
                        emu->registers[dst] *= immn;
                        debugImmn(emu, "mul", immn, dst);
                        break;
                    case 0x5:
                        emu->registers[dst] *= emu->registers[src];
                        debug(emu, "mul", src, dst);
                        break;
                    case 0x6:
                        emu->registers[dst] /= immn;
                        debugImmn(emu, "div", immn, dst);
                        break;
                    case 0x7:
                        emu->registers[dst] /= emu->registers[src];
                        debug(emu, "div", src, dst);
                        break;
                    case 0xa:
                        emu->registers[dst] = immn;
                        debugImmn(emu, "mov", immn, dst);
                        break;
                    case 0xb:
                        emu->registers[dst] = emu->registers[src];
                        debug(emu, "mov", src, dst);
                        break;
                    case 0xc:
                        emu->registers[dst]++;
                        debugr(emu, "inc", dst);
                        break;
                    case 0xd:
                        emu->registers[dst]--;
                        debugr(emu, "dec", dst);
                        break;
                    case 0x10:
                        emu->registers[dst] = getWord(emu, immn);
                        debugImmn(emu, "ldr", immn, dst);
                        break;
                    case 0x12:
                        emu->registers[dst] = getWord(emu, immn+emu->registers[IX]);
                        debugImmn(emu, "ldx", immn, dst);
                        break;
                    case 0x14:
                        emu->registers[dst] <<= immn;
                        debugImmn(emu, "asl", immn, dst);
                        break;
                    case 0x15:
                        emu->registers[dst] <<= emu->registers[src];
                        debug(emu, "asl", src, dst);
                        break;
                    case 0x16:
                        emu->registers[dst] >>= immn;
                        debugImmn(emu, "asr", immn, dst);
                        break;
                    case 0x17:
                        emu->registers[dst] >>= emu->registers[src];
                        debug(emu, "asr", src, dst);
                        break;
                    case 0x1a:
                        emu->registers[dst] = emu->registers[SP];
                        debugr(emu, "gsp", dst);
                        break;
                    case 0x1b:
                        emu->registers[dst] |= immn;
                        debugImmn(emu, "or", immn, dst);
                        break;
                    case 0x1c:
                        emu->registers[dst] |= emu->registers[src];
                        debug(emu, "or", src, dst);
                        break;
                    case 0x1d:
                        emu->registers[dst] &= immn;
                        debugImmn(emu, "and", immn, dst);
                        break;
                    case 0x1e:
                        emu->registers[dst] &= emu->registers[src];
                        debug(emu, "and", src, dst);
                        break;
                    case 0x1f:
                        emu->registers[dst] = ~emu->registers[dst];
                        debugr(emu, "not", dst);
                        break;
                    case 0x20:
                        emu->registers[dst] ^= immn;
                        debugImmn(emu, "xor", immn, dst);
                        break;
                    case 0x21:
                        emu->registers[dst] ^= emu->registers[src];
                        debug(emu, "xor", src, dst);
                        break;
                    case 0x22:
                        temp = emu->registers[src];
                        emu->registers[dst] = (emu->registers[FL] & (1UL>>temp)) >> temp;
                        debugImmn(emu, "flg", immn, dst);
                        break;
                    case 0x26:
                        emu->registers[dst] = pop(emu);
                        debugr(emu, "pop", dst);
                        break;
                    case 0x2b:
                        setWord(emu, immn+emu->registers[IX], pop(emu));
                        debugi(emu, "popx", immn);
                        break;
                    case 0x2c:
                        emu->registers[dst] = emu->registers[CL];
                        debugr(emu, "clc", dst);
                        break;
                }

                arithSetFlags(emu, emu->registers[dst]);

            //  
            //  STACK PUSH INSTRUCTIONS
            //  
            } else if (op >= 0x23 && op <= 0x25) {
                if (op != 0x24) {
                    immn = getWord(emu, emu->registers[PC]);
                    emu->registers[PC] += 2;
                    emu->tickCounter++;
                }
                emu->tickCounter++;

                switch (op) {
                    case (0x23):
                        push(emu, immn);
                        debugi(emu, "push", immn);
                        break;
                    case (0x24):
                        push(emu, emu->registers[src]);
                        debugs(emu, "push", immn, src);
                        break;
                    case (0x25):
                        push(emu, immn+emu->registers[IX]);
                        debugs(emu, "push", immn, src);
                        break;
                }
            }
            
            //  
            //  MISC OTHER STUFF
            //  
            else {
                if (op == 0x0E || op == 0x11 || op == 0x13 || op == 0x18 || op == 0x30 || op == 0x31) {
                    immn = getWord(emu, emu->registers[PC]);
                    emu->registers[PC] += 2;
                    emu->tickCounter++;
                }

                switch (op) {
                    case 0x0E:
                        emu->registers[FL] = setBit(emu->registers[FL], FL_EQ, emu->registers[src] == immn);
                        emu->registers[FL] = setBit(emu->registers[FL], FL_LT, emu->registers[src] < immn);
                        emu->registers[FL] = setBit(emu->registers[FL], FL_GT, emu->registers[src] > immn);
                        debugImmn(emu, "cmp", immn, dst);
                        break;
                    case 0x0F:  // CMPR; 1 word
                        emu->registers[FL] = setBit(emu->registers[FL], FL_EQ, emu->registers[src] == emu->registers[dst]);
                        emu->registers[FL] = setBit(emu->registers[FL], FL_LT, emu->registers[src] < emu->registers[dst]);
                        emu->registers[FL] = setBit(emu->registers[FL], FL_GT, emu->registers[src] > emu->registers[dst]);
                        debug(emu, "cmp", src, dst);
                        break;
                    case 0x11:  // STR; 2 words
                        setWord(emu, immn, emu->registers[src]);
                        debugs(emu, "str", immn, dst);
                        break;
                    case 0x13:  // STX; 2 words
                        setWord(emu, immn+emu->registers[IX], emu->registers[src]);
                        if (emu->clargs&1) printf("%04x  stx r%x -> 0x%04x\n", immn, src, immn);
                        break;

                    case 0x18:  // SSP; 2 words
                        emu->registers[SP] = immn;
                        if (emu->clargs&1) printf("%04x  ssp 0x%04x\n", immn, immn);
                        break;
                    case 0x19:  // SSPR; 1 word
                        emu->registers[SP] = emu->registers[src];
                        if (emu->clargs&1) printf("      sspr r%x\n", src);
                        break;

                    case 0x30:  // JMP; 2 words
                        emu->registers[PC] = immn;
                        if (emu->clargs&1) printf("%04x  jmp 0x%04x\n", immn, immn);
                        break;
                    case 0x31:  // JSR; 2 words
                        push(emu, emu->registers[PC]);
                        emu->registers[PC] = immn;
                        if (emu->clargs&1) printf("%04x  jsr 0x%04x\n", immn, immn);
                        break;
                    case 0x32:  // RTS; 1 words
                        emu->registers[PC] = pop(emu);
                        if (emu->clargs&1) printf("      rts\n");
                        break;

                    case 0x3F:  // EXIT; 1 word
                        emu->registers[FL] |= 1;
                        if (emu->clargs&1) printf("      exit\n");
                        break;
                    default:
                        printf("invalid opcode %02x\n", op);
                        break;
                }
            }
        } else {
            if (emu->clargs&1) printf("      Condition failed\n");
        
            if (op == 0x00 || op == 0x02 ||
                op == 0x04 || op == 0x06 ||
                op == 0x0a || op == 0x0e ||
                op == 0x10 || op == 0x11 ||
                op == 0x12 || op == 0x13 ||
                op == 0x14 || op == 0x16 ||
                op == 0x18 || op == 0x1b ||
                op == 0x1d || op == 0x20 ||
                op == 0x22 || op == 0x23 ||
                op == 0x25 || op == 0x27 ||
                op == 0x29 || op == 0x30 ||
                op == 0x31 || op == 0x35) emu->registers[PC] += 2;
        }

        if ((emu->clargs&0b00000010)>>1) {
            getchar();
            printf("\x1B[1A");
        }
    }
}