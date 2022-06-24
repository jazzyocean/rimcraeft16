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
    
    const int countW = 80;
    const int countH = 25;
    const int charsizeH = 20;
    const int charsizeW = 10;
    const int screenWidth = countW*charsizeW;
    const int screenHeight = countH*charsizeH;
    int posx = 0;
    int posy = 0;
    int row = 0;
    int col = 0;
    uint16_t offs = 0;
    int keysLastPressed[6];
    int ckey = 0;

    SetTraceLogLevel(LOG_NONE);
    InitWindow(screenWidth, screenHeight, "RIMCRÆFT VIDEO");
    SetTargetFPS(60);

    Font font = LoadFontEx("src\\emu\\resources\\Px437_IBM_VGA_8x16.ttf", charsizeH, 0, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
    
    clock_t prev = clock();
    while (testBit(emu->registers[FL], FL_OF, 0) && !WindowShouldClose()) {
        if (((double)(clock() - prev))/CLOCKS_PER_SEC >= 0.01) {
            prev = clock();
            posx = 0;
            posy = 0;
            row = 0;
            col = 0;
            offs = 0;
            BeginDrawing();
                ClearBackground((Color){0, 0, 0, 255});
                while (row < countH) {
                    // Get current char as string
                    char str[2];
                    str[0] = emu->memory[(offs+VRAM)&0xFFFF];
                    str[1] = '\0';    
                    char c = str[0];

                    // If the character is a newline, move to the next line
                    if (c == '\n') {
                        posy += charsizeH;
                        posx = 0;
                        col = 0;
                        row++;
                        offs++;
                        continue;
                    }
                    
                    // Else, draw the character
                    DrawTextEx(font, str, (Vector2){posx, posy}, charsizeH, 0, (Color){255, 255, 255, 255});
                    posx += charsizeW;
                    offs++;
                    if (offs == 0xFFFF || offs == 0x0000) {
                        offs = 0;
                    }

                    col++;
                    // If col is greater than 80, move to the next line
                    if (col >= countW) {
                        posy += charsizeH;
                        posx = 0;
                        row++;
                        col = 0;
                    }
                    // If posy is greater than 25, stop drawing
                    if (posy >= screenHeight) {
                        break;
                    }

                }
            EndDrawing();

            int key;

            for (int keylp = 0; keylp < 6; keylp++) {
                if (keysLastPressed[keylp] == 0) continue;
                if (!IsKeyDown(keysLastPressed[keylp])) {
                    keysLastPressed[ckey] = 0;
                    ckey--;
                    setWord(emu, 0x98F8, (uint16_t)keysLastPressed[keylp]);
                    emu->registers[FL] |= (1 << FL_INT);
                    emu->intID = 0x02;
                }
            }

            do {
                key = GetKeyPressed();
                if (ckey < 6) keysLastPressed[ckey] = key;
                if (key != 0) {
                    setWord(emu, 0x98F8, (uint16_t)key);
                    emu->registers[FL] |= (1 << FL_INT);
                    emu->intID = 0x01;
                    ckey++;
                }
            } while (key != KEY_NULL);
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
    

    emu->registers[PC] = getWord(emu, ROM_0+S0_ENTRY_POINT);
    printf(INFO_TAG"   : ROM Flags: 0x%04x\n", getWord(emu, ROM_0+S0_ROM_FLAGS));
    printf(INFO_TAG"   : Num Sectors in ROM: 0x%04x\n", getWord(emu, ROM_0+S0_NUM_SECTORS));
    printf(INFO_TAG"   : Entry Point: 0x%04x\n", getWord(emu, ROM_0+S0_ENTRY_POINT));

    emu->startTime = clock();
}

void debugMessageImmnDst(Emulator *emu, char *opc, uint16_t immn, int dst) {
    if (emu->clargs & 1) printf("%04x  %s 0x%04x -> r%x\n", immn, opc, immn, dst);
}
void debugMessageImmn(Emulator *emu, char *opc, uint16_t immn) {
    if (emu->clargs & 1) printf("%04x  %s 0x%04x\n", immn, opc, immn);
}
void debugMessageSrcDst(Emulator *emu, char *opc, int src, int dst) {
    if (emu->clargs & 1) printf("      %s r%x -> r%x\n", opc, src, dst);
}
void debugMessageDst(Emulator *emu, char *opc, int dst) {
    if (emu->clargs & 1) printf("      %s r%x\n", opc, dst);
}
void debugMessageSrc(Emulator *emu, char *opc, uint16_t immn, int src) {
    if (emu->clargs & 1) printf("%04x  %s r%x -> 0x%04x\n", immn, opc, src, immn);
}
void debugMessage(Emulator *emu, char *opc) {
    if (emu->clargs & 1) printf("      %s\n", opc);
}

// Mainloop of Emulator emulation
void emulationLoop(Emulator *emu) {
    // While flag 0 is 0
    while (testBit(emu->registers[FL], FL_OF, 0)) {
        
        uint16_t inst = getWord(emu, emu->registers[PC]);

        // FEDCBA98 76543210
        // dddcccco ooooosss
        // d: dst
        // c: condition
        // o: opcode
        // s: src

        int src = inst & 0b111;
        int op = (inst >> 3) & 0b111111;
        int cn = (inst >> 9) & 0b1111;
        int dst = (inst >> 0xD) & 0b111;
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
            if (op <= 0x7 || (op >= 0xa && op <= 0xd) 
                          || op == 0x10 || op == 0x12
                          || (op >= 0x14 && op <= 0x18)
                          || (op >= 0x1a && op <= 0x22)
                          || (op >= 0x26 && op <= 0x2d)
                          || op == 0x39 || op == 0x3A) {
                if (op == 0x00 || op == 0x02 || op == 0x04 || op == 0x06 ||
                    op == 0x0a || op == 0x10 || op == 0x12 || op == 0x14 ||
                    op == 0x16 || op == 0x1b || op == 0x1d || op == 0x20 ||
                    op == 0x22 || op == 0x27 || op == 0x29 || op == 0x2b) {
                    immn = getWord(emu, emu->registers[PC]);
                    emu->registers[PC] += 2;
                    emu->tickCounter += 2;
                }

                switch (op) {
                    case 0x0:
                        emu->registers[dst] += immn;
                        debugMessageImmnDst(emu, "add", immn, dst);
                        break;
                    case 0x1:
                        emu->registers[dst] += emu->registers[src];
                        debugMessageSrcDst(emu, "add", src, dst);
                        break;
                    case 0x2:
                        emu->registers[dst] -= immn;
                        debugMessageImmnDst(emu, "sub", immn, dst);
                        break;
                    case 0x3:
                        emu->registers[dst] -= emu->registers[src];
                        debugMessageSrcDst(emu, "sub", src, dst);
                        break;
                    case 0x4:
                        emu->registers[dst] *= immn;
                        debugMessageImmnDst(emu, "mul", immn, dst);
                        break;
                    case 0x5:
                        emu->registers[dst] *= emu->registers[src];
                        debugMessageSrcDst(emu, "mul", src, dst);
                        break;
                    case 0x6:
                        emu->registers[dst] /= immn;
                        debugMessageImmnDst(emu, "div", immn, dst);
                        break;
                    case 0x7:
                        emu->registers[dst] /= emu->registers[src];
                        debugMessageSrcDst(emu, "div", src, dst);
                        break;
                    case 0xa:
                        emu->registers[dst] = immn;
                        debugMessageImmnDst(emu, "mov", immn, dst);
                        break;
                    case 0xb:
                        emu->registers[dst] = emu->registers[src];
                        debugMessageSrcDst(emu, "mov", src, dst);
                        break;
                    case 0xc:
                        emu->registers[dst]++;
                        debugMessageDst(emu, "inc", dst);
                        break;
                    case 0xd:
                        emu->registers[dst]--;
                        debugMessageDst(emu, "dec", dst);
                        break;
                    case 0x10:
                        emu->registers[dst] = getWord(emu, immn);
                        debugMessageImmnDst(emu, "ldr", immn, dst);
                        break;
                    case 0x12:
                        emu->registers[dst] = getWord(emu, immn+emu->registers[IX]);
                        debugMessageImmnDst(emu, "ldx", immn, dst);
                        break;
                    case 0x14:
                        emu->registers[dst] <<= immn;
                        debugMessageImmnDst(emu, "lsl", immn, dst);
                        break;
                    case 0x15:
                        emu->registers[dst] <<= emu->registers[src];
                        debugMessageSrcDst(emu, "lsl", src, dst);
                        break;
                    case 0x16:
                        emu->registers[dst] >>= immn;
                        debugMessageImmnDst(emu, "lsr", immn, dst);
                        break;
                    case 0x17:
                        emu->registers[dst] >>= emu->registers[src];
                        debugMessageSrcDst(emu, "lsr", src, dst);
                        break;
                    case 0x1a:
                        emu->registers[dst] = emu->registers[SP];
                        debugMessageDst(emu, "gsp", dst);
                        break;
                    case 0x1b:
                        emu->registers[dst] |= immn;
                        debugMessageImmnDst(emu, "or", immn, dst);
                        break;
                    case 0x1c:
                        emu->registers[dst] |= emu->registers[src];
                        debugMessageSrcDst(emu, "or", src, dst);
                        break;
                    case 0x1d:
                        emu->registers[dst] &= immn;
                        debugMessageImmnDst(emu, "and", immn, dst);
                        break;
                    case 0x1e:
                        emu->registers[dst] &= emu->registers[src];
                        debugMessageSrcDst(emu, "and", src, dst);
                        break;
                    case 0x1f:
                        emu->registers[dst] = ~emu->registers[dst];
                        debugMessageDst(emu, "not", dst);
                        break;
                    case 0x20:
                        emu->registers[dst] ^= immn;
                        debugMessageImmnDst(emu, "xor", immn, dst);
                        break;
                    case 0x21:
                        emu->registers[dst] ^= emu->registers[src];
                        debugMessageSrcDst(emu, "xor", src, dst);
                        break;
                    case 0x22:
                        temp = emu->registers[src];
                        emu->registers[dst] = (emu->registers[FL] & (1UL>>temp)) >> temp;
                        debugMessageImmnDst(emu, "flg", immn, dst);
                        break;
                    case 0x26:
                        emu->registers[dst] = pop(emu);
                        debugMessageDst(emu, "pop", dst);
                        break;
                    case 0x2b:
                        emu->registers[dst] = emu->memory[immn];
                        debugMessageImmnDst(emu, "ldb", immn, dst);
                        break;
                    case 0x2c:
                        emu->registers[dst] = emu->registers[CL];
                        debugMessageDst(emu, "clc", dst);
                        break;
                    case 0x2d:
                        emu->registers[dst] = emu->memory[emu->registers[src]];
                        debugMessageSrcDst(emu, "ldb", src, dst);
                        break;
                    case 0x39:;
                        uint16_t temp = emu->registers[dst];
                        emu->registers[dst] = emu->registers[src];
                        emu->registers[src] = temp;
                        debugMessageSrcDst(emu, "swap", src, dst);
                        break;
                    case 0x3a:
                        emu->registers[dst] = getWord(emu, emu->registers[src]);
                        debugMessageSrcDst(emu, "ldr", src, dst);
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
                        debugMessageImmn(emu, "push", immn);
                        break;
                    case (0x24):
                        push(emu, emu->registers[src]);
                        debugMessageDst(emu, "push", src);
                        break;
                    case (0x25):
                        push(emu, immn+emu->registers[IX]);
                        debugMessageSrc(emu, "push", immn, src);
                        break;
                }
            }
            
            //  
            //  MISC OTHER STUFF
            //  
            else {
                if (op == 0x0E || op == 0x11 ||
                    op == 0x13 || op == 0x18 ||
                    op == 0x30 || op == 0x31 ||
                    op == 0x35 || op == 0x3c) {
                    immn = getWord(emu, emu->registers[PC]);
                    emu->registers[PC] += 2;
                    emu->tickCounter++;
                }

                switch (op) {
                    case 0x08:  // SCP; 1 words
                        emu->registers[FL] |= 1 << FL_CP;
                        debugMessage(emu, "scp");
                        break;
                    case 0x09:  // CCP; 1 words
                        emu->registers[FL] &= ~(1 << FL_CP);
                        debugMessage(emu, "ccp");
                        break;

                    case 0x0E:
                        emu->registers[FL] = setBit(emu->registers[FL], FL_EQ, emu->registers[src] == immn);
                        emu->registers[FL] = setBit(emu->registers[FL], FL_LT, emu->registers[src] < immn);
                        emu->registers[FL] = setBit(emu->registers[FL], FL_GT, emu->registers[src] > immn);
                        debugMessageImmnDst(emu, "cmp", immn, dst);
                        break;
                    case 0x0F:  // CMPR; 1 word
                        emu->registers[FL] = setBit(emu->registers[FL], FL_EQ, emu->registers[src] == emu->registers[dst]);
                        emu->registers[FL] = setBit(emu->registers[FL], FL_LT, emu->registers[src] < emu->registers[dst]);
                        emu->registers[FL] = setBit(emu->registers[FL], FL_GT, emu->registers[src] > emu->registers[dst]);
                        debugMessageSrcDst(emu, "cmp", src, dst);
                        break;
                    case 0x11:  // STR; 2 words
                        setWord(emu, immn, emu->registers[src]);
                        debugMessageSrc(emu, "str", immn, dst);
                        break;
                    case 0x13:  // STX; 2 words
                        setWord(emu, immn+emu->registers[IX], emu->registers[src]);
                        debugMessageSrc(emu, "stx", immn, src);
                        break;

                    case 0x2e:  // SCR; 1 words
                        emu->registers[FL] |= 1 << FL_CR;
                        debugMessage(emu, "scr");
                        break;
                    case 0x2f:  // CLCR; 1 words
                        emu->registers[FL] &= ~(1 << FL_CR);
                        debugMessage(emu, "clcr");
                        break;

                    case 0x18:  // SSP; 2 words
                        emu->registers[SP] = immn;
                        debugMessageImmn(emu, "ssp", immn);
                        break;
                    case 0x19:  // SSPR; 1 word
                        emu->registers[SP] = emu->registers[src];
                        debugMessageDst(emu, "sspr", dst);
                        break;

                    case 0x30:  // JMP; 2 words
                        emu->registers[PC] = immn;
                        debugMessageImmn(emu, "jmp", immn);
                        break;
                    case 0x31:  // JSR; 2 words
                        push(emu, emu->registers[PC]);
                        emu->registers[PC] = immn;
                        debugMessageImmn(emu, "jsr", immn);
                        break;
                    case 0x32:  // RTS; 1 words
                        emu->registers[PC] = pop(emu);
                        debugMessage(emu, "rts");
                        break;

                    case 0x35:  // INT; 2 words
                        emu->intID = immn;
                        emu->registers[FL] |= 1 << FL_INT;
                        debugMessageImmn(emu, "int", immn);
                        break;

                    case 0x36:  // STI; 1 words
                        emu->registers[FL] |= 1 << FL_IN;
                        debugMessage(emu, "sti");
                        break;
                    case 0x37:  // CLI; 1 word
                        emu->registers[FL] &= ~(1 << FL_IN);
                        debugMessage(emu, "cli");
                        break;

                    case 0x3b:  // STR; 2 words
                        setWord(emu, emu->registers[dst], emu->registers[src]);
                        debugMessageSrcDst(emu, "str", src, dst);
                        break;
                    
                    case 0x3c:  // STB; 2 words
                        emu->memory[immn] = emu->registers[src] & 0xFF;
                        debugMessageSrc(emu, "stb", immn, src);
                        break;
                    case 0x3d:  // STB; 2 words
                        emu->memory[emu->registers[dst]] = emu->registers[src] & 0xFF;
                        debugMessageSrc(emu, "stb", immn, src);
                        break;
                    
                    case 0x3E:
                        if ((emu->clargs & 0b00000100) >> 2) {
                            emu->breakpoint = !emu->breakpoint;
                            debugMessage(emu, "brk");
                        }
                        break;
                    
                    case 0x3F:  // Exit; 1 word
                        emu->registers[FL] |= 1 << FL_OF;
                        debugMessage(emu, "exit");
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
                op == 0x29 || op == 0x2b ||
                op == 0x30 || op == 0x31 ||
                op == 0x33 || op == 0x35 ||
                op == 0x3c) emu->registers[PC] += 2;
        }

        if (emu->breakpoint) {
            getchar();
            printf("\x1B[1A");
        }

        if (testBit(emu->registers[FL], FL_IN, 0) && testBit(emu->registers[FL], FL_INT, 1)) {
            push(emu, emu->registers[PC]);
            uint16_t sector = getWord(emu, 0x4000+(4*emu->intID)+2);
            if (sector != 0) loadSector(emu, sector, 3);
            emu->registers[FL] &= ~(1 << FL_INT);
            emu->registers[PC] = getWord(emu, 0x4000+(4*emu->intID));
            if (emu->clargs&1) printf(DEBUG_TAG"INT 0x%02x\n", emu->intID);
        }

        if (testBit(emu->registers[FL], FL_CP, 1)) {
            if (emu->clargs & 1) printf(INFO_TAG"Checking ROM paging and RAM banking\n");
            const uint16_t wramsel0 = getWord(emu, 0x9B02);
            const uint16_t wramsel1 = getWord(emu, 0x9B04);
            const uint16_t wramsel2 = getWord(emu, 0x9B06);
            const uint16_t wramsel3 = getWord(emu, 0x9B08);
            const uint16_t romsel0 = getWord(emu, 0x9B0A);
            const uint16_t romsel1 = getWord(emu, 0x9B0C);
            const uint16_t romsel2 = getWord(emu, 0x9B0E);
            const uint16_t romsel3 = getWord(emu, 0x9B10);
            if (emu->clargs & 1) printf(INFO_TAG"WRAMSEL0-3: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
                                        wramsel0, wramsel1, wramsel2, wramsel3);
            if (emu->clargs & 1) printf(INFO_TAG"ROMSEL0-3 : 0x%04x, 0x%04x, 0x%04x, 0x%04x\n",
                                        romsel0, romsel1, romsel2, romsel3);

            if (romsel0 != emu->prevRomSel[0]) {
                loadSector(emu, romsel0, 0);
                emu->prevRomSel[0] = romsel0;
                if (emu->clargs & 1) printf(MEMORY_TAG"Paged sector 0x%04x to slot 0\n", romsel0);
                if (emu->clargs & 1) printSector(&emu->memory[0x0000], 0);
            }

            if (romsel1 != emu->prevRomSel[1]) {
                loadSector(emu, romsel1, 1);
                emu->prevRomSel[1] = romsel1;
                if (emu->clargs & 1) printf(MEMORY_TAG"Paged sector 0x%04x to slot 1\n", romsel1);
                if (emu->clargs & 1) printSector(&emu->memory[0x1000], 1);
            }

            if (romsel2 != emu->prevRomSel[2]) {
                loadSector(emu, romsel2, 2);
                emu->prevRomSel[2] = romsel2;
                if (emu->clargs & 1) printf(MEMORY_TAG"Paged sector 0x%04x to slot 2\n", romsel2);
                if (emu->clargs & 1) printSector(&emu->memory[0x2000], 2);
            }

            if (romsel3 != emu->prevRomSel[3]) {
                loadSector(emu, romsel3, 3);
                emu->prevRomSel[3] = romsel3;
                if (emu->clargs & 1) printf(MEMORY_TAG"Paged sector 0x%04x to slot 3\n", romsel3);
                if (emu->clargs & 1) printSector(&emu->memory[0x3000], 3);
            }
        }
    }
}