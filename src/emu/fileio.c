#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "emu.h"


uint8_t *copy(char *filePath) {
    FILE *fp = fopen(filePath, "r");
    if (fp == NULL) {
        printf("File not found: %s\n", filePath);
        return NULL;
    }

    uint8_t sectorsArr[2];
    fseek(fp, 0x0FFE, SEEK_SET);
    if (fread(sectorsArr, sizeof(uint8_t), 2, fp) == 0) {
        printf(ERROR_TAG"There was an error reading from %s\n", filePath);
        return NULL;
    }
    fseek(fp, 0, SEEK_SET);
    uint16_t numSectors = sectorsArr[0] | (sectorsArr[1] << 8);
    
    uint8_t *buf = calloc(0x1000*numSectors, sizeof(uint8_t));
    unsigned long nbytes = fread(buf, sizeof(uint8_t), 0x1000*numSectors, fp);
    if (nbytes == 0) {
        printf(ERROR_TAG"There was an error copying ROM data\n");
        return NULL;
    }
    printf(INFO_TAG"Read %d (0x%07x) bytes of ROM to buffer\n", nbytes, nbytes);

    return buf;
}