#include <stdint.h>

#ifndef ROM_H
#define ROM_H

int copyPS(uint8_t *buf, char *filePath); // PLatform-specific, allows for windows to use drives as files.
uint8_t *copy(char *filePath);

#endif