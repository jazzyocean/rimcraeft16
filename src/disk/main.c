#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// 
//  TODO: Implement automatic filesystem creation.
// 


uint8_t *copy(char *filePath) {
    FILE *fp = fopen(filePath, "r");
    if (fp == NULL) {
        printf("File not found: %s\n", filePath);
        return NULL;
    }

    uint8_t sectorsArr[2];
    fseek(fp, 0x0FFE, SEEK_SET);
    if (fread(sectorsArr, sizeof(uint8_t), 2, fp) == 0) {
        printf("There was an error reading from %s\n", filePath);
        return NULL;
    }
    fseek(fp, 0, SEEK_SET);
    uint16_t numSectors = sectorsArr[0] | (sectorsArr[1] << 8);
    
    uint8_t *buf = calloc(0x1000*numSectors, sizeof(uint8_t));
    unsigned long nbytes = fread(buf, sizeof(uint8_t), 0x1000*numSectors, fp);
    if (nbytes == 0) {
        printf("There was an error copying ROM data\n");
        return NULL;
    }
    printf("Read %d (0x%07x) bytes of ROM to buffer\n", nbytes, nbytes);

    return buf;
}

// Display 0x1000 byte long bytearray, return number of lines read
int printSector(uint8_t *sector, uint16_t csector) {
    char current[114] = {0};
    char previous[114] = {0};
    int skipped = 0;
    int i;
    int lines;

    lines++;
    printf("\n");
    printf("           -0 -1 -2 -3 -4 -5 -6 -7  -8 -9 -a -b -c -d -e -f    ---0 ---2 ---4 ---6  ---8 ---a ---c ---e\n");

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
            printf("%04x:%04x  %s\n", csector, i, current);
            strcpy(previous, current);
            skipped = 0;
            lines++;
        }
        else if (res == 0 && !skipped){
            // If it is, and the previous line wasnt skipped, display stars
            skipped = 1;
            printf("           *  *  *\n");
            lines++;
        }
        // If it is, and the previous line was skipped then like...idk man go fuck yourself
    }

    // If we skipped the last lines, show where the end bytes are
    // Not really useful for this function's specific use but im adding it anyways
    if (skipped) {
        printf("%04x:%04x\n", csector, i);
        lines++;
    }
    printf("\n");
    lines++;

    return lines;
}

int main(int argc, char **argv) {
    uint8_t *buf;

    // if no file is inputted, set buf to a 0x1000 long bytearray
    // else, set buf to the return value of copy()
    if (argc == 1) {
        buf = calloc(0x1000, sizeof(uint8_t));
        printf("No file inputted, using an empty 0x1000 long bytearray\n");
    } else {
        buf = copy(argv[1]);
    }

    int cont = 1;
    int lines = 0;
    int sector = 0;
    int buflength = 0x1000 * (buf[0x0FFE] | (buf[0x0FFF] << 8));
    char input[64];
     
    while (cont) {
        // Print buf
        lines += printSector(&buf[sector * 0x1000], sector);
        char c = getchar();
        printf("\n");
        switch (c) {
            case 'q':
                cont = 0;
                break;
            case '>':
                sector++;
                break;
            case '<':
                sector--;
                break;
            // if user typed :, get the next line and interpret it as a sector number as a hex number between 0x0000 and 0xFFFF
            case ':':
                printf("\n");
                printf("Enter sector number: ");
                scanf("%s", input);
                sector = strtol(input, NULL, 16);
                break;
            // if ?, show a list of commands
            case '?':

                printf("\n");
                printf("Commands:\n");
                printf("q: quit\n");
                printf("<: go to previous sector\n");
                printf(">: go to next sector\n");
                printf(":: go to sector number\n");
                printf("?: show this help\n");
                // wait for user input to continue
                printf("Press enter to continue\n");
                fflush(stdin);
                getchar();
                printf("\033[2J\033[1;1H");
            // if a, input file path, and calculate number of bytes in that file, reallocate buf, then append the file to buf
            case 'i':
                printf("\n");
                printf("Enter file path: ");
                scanf("%s", input);
                FILE* fp = fopen(input, "rb");
                if (fp == NULL) {
                    printf("File not found\n");
                    break;
                }
                fseek(fp, 0, SEEK_END);
                int size = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                buflength += size;
                buf = realloc(buf, buflength);
                fread(&buf[buflength - size], size, 1, fp);
                fclose(fp);
                printf("Appended %d bytes from %s\n", size, input);

                // wait for user input to continue
                printf("Press enter to continue\n");
                fflush(stdin);
                getchar();
                printf("\033[2J\033[1;1H");
                break;
            // if s, save buf to a file
            case 's':
                printf("\n");
                printf("Enter file path: ");
                scanf("%s", input);
                FILE* fp2 = fopen(input, "wb");
                if (fp2 == NULL) {
                    printf("File not found\n");
                    break;
                }
                fwrite(buf, buflength, 1, fp2);
                fclose(fp2);
                printf("Saved to %s\n", input);
                // wait for user input to continue
                printf("Press enter to continue\n");
                fflush(stdin);
                getchar();
                printf("\033[2J\033[1;1H");
                break;
            // if i, insert a file into buf at the current sector, reallocate only if necessary
            case 'a':
                printf("\n");
                printf("Enter file path: ");
                scanf("%s", input);
                FILE* fp3 = fopen(input, "rb");
                if (fp3 == NULL) {
                    printf("File not found\n");
                    break;
                }
                fseek(fp3, 0, SEEK_END);
                int size2 = ftell(fp3);
                fseek(fp3, 0, SEEK_SET);
                if ((size2 + 0x1000*(sector-1)) > buflength) {
                    buf = realloc(buf, buflength + size2);
                    buflength += size2;
                }
                fread(&buf[(sector-1) * 0x1000], size2, 1, fp3);
                fclose(fp3);
                printf("Inserted %d bytes from %s\n", size2, input);
                // wait for user input to continue
                printf("Press enter to continue\n");
                fflush(stdin);
                getchar();
                printf("\033[2J\033[1;1H");
                break;
            

            // Clear screen
            case '\n':
                printf("\033[2J\033[1;1H");
                break;
            default:
                printf("%c", c);
                break;
        

        }
    }
}