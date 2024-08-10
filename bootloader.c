// This is basically a simualtion in parsing an SREC file that would load your program.
#include <stdio.h>
#include <stdlib.h>

#define DEBUG_OUTPUT 1

FILE *fp;
void init_file() {
    char *file = "notmain.srec";

    fp = fopen(file, "r");
    if (fp == NULL) {
        fprintf(stderr, "error opening file %s\n", file);
        exit(1);
    }
}

// Simulate functions for using the UART
#define uart_init() init_file()
#define uart_send(...)  putchar(__VA_ARGS__)
#define hexstring(fmt, ...) printf(fmt, __VA_ARGS__)

int uart_recv() {
    unsigned int ra = fgetc(fp);
    return ra != EOF ? ra : 0;
}

void PUT8(unsigned int address, unsigned char data) {
    printf("Writing 0x%02X to 0x%08X\n", data, address); 
}

// byte to nibblet conversion
static unsigned int ctonib (unsigned int c) {
    if (c > 0x39) c-=7;
    return (c & 0xF);
}

typedef struct srec_decode {
    unsigned char type[2];
    unsigned int count;
    unsigned int addr;
    unsigned int data;
    unsigned int checksum;
} srec_decode;

// 0x0D - '\r'
// 0x0A - '\n'
int main(void) 
{ 
    uart_init();

    uart_send(0x0D);
    uart_send(0x0A);

    unsigned int ra;
    unsigned int state;
    int i = 0;
    unsigned int addr_count = 0;
    unsigned int data_count = 0;
    srec_decode sbug = {0};

    uart_send('S');
    uart_send('R');
    uart_send('E');
    uart_send('C');
    uart_send(0x0D);
    uart_send(0x0A);
    uart_send(0x0D);
    uart_send(0x0A);

    unsigned int entry_start = 0x8000; // entry point of program to load

    // Note, we would usually sit in this loop forever while the program runs - we will return for sim purposes
    while ((ra = uart_recv())) {
        // Begining of SREC line to process
        if (ra == 'S') {
            sbug.type[0] = 'S';
            sbug.checksum = 0;
            state=1;
            continue;
        }

        if (ra == 0x0D || ra == 0x0A) {
            state = 0;
            continue;
        }

        switch (state)
        {
            case 0:
                data_count = 0;
                addr_count = 0;
                break;
            case 1:
                switch (ra) 
                {
                    case '0':
                        sbug.type[1] = '0';
                        state++;
                        break;
                    case '3':
                        sbug.type[1] = '3';
                        state++;
                        break;
                    case '7':
                        sbug.type[1] = '7';
                        hexstring("%08X\n",0xDEADDEAD);
                        state++;
                        break;
                    default:
                        hexstring("%c\n", ra);
                        hexstring("%08X\n", 0xBADBAD01);
                        return 1;
                }
                break;
            case 2:
#if DEBUG_OUTPUT
                printf("================================\n");
                printf("Processing Byte Count section...\n");
#endif
                sbug.count = ctonib(ra);
                state++;
                break;
            case 3:
                sbug.count <<= 4;
                sbug.count |= ctonib(ra);
#if DEBUG_OUTPUT
                hexstring("Byte Count: 0x%X\n", sbug.count);
#endif
                sbug.checksum += (sbug.count&0xFF);
                sbug.addr=0;
                data_count = sbug.count*2; // number of hex digits to process
                addr_count = 0;
#if DEBUG_OUTPUT
                printf("Processing Address section...\n");
                hexstring("Hexdigits to Process: %d\n", data_count);
#endif
                state++;
                break;
            case 4:
                if (addr_count == 8) { // Finished processing address section
                    sbug.data = ctonib(ra);
                    data_count--;
                    state++;
                } else {
                    sbug.addr <<= 4;
                    sbug.addr |= ctonib(ra);
                    // accumulate checksum on a hexdigit pair basis
                    if (addr_count % 2) { 
                        sbug.checksum += (sbug.addr&0xFF);
#if DEBUG_OUTPUT
                        printf("checksum: 0x%X address:  0x%X\n", sbug.checksum, sbug.addr&0xFF);
#endif
                    }
                    addr_count++;
#if DEBUG_OUTPUT
                    if (addr_count == 8) {
                        printf("Processing Data section...\n");
                        hexstring("Hexdigits to Process: %d\n", data_count-1);
                    }
#endif
                    data_count--;
                }
                break;
            case 5:
                if (data_count == 1) { // Indicates end of current line of srec line (i.e we are at the checksum byte)
                                       // Don't load the checksum byte into the program (i.e dont PUT8())
                    sbug.checksum = 0xFF - (sbug.checksum & 0xFF);
#if DEBUG_OUTPUT
                    printf("Calculating Final Checksum...\n");
                    hexstring("ADDRESS: 0x%08X ========== 0x%02X\n\n", sbug.addr, sbug.checksum&0xFF);
#endif
                    state = 0;
                } else {
                    sbug.data <<= 4;
                    sbug.data |= ctonib(ra);
                    if (data_count > 0) {
                        if (data_count % 2) { // accumulate checksum on a hexdigit pair basis
                            sbug.checksum += (sbug.data&0xFF);
#if DEBUG_OUTPUT
                            hexstring("checksum: 0x%X data: 0x%X\n", sbug.checksum, sbug.data&0xFF);
#endif
                            i++;
                        }
#ifndef DEBUG_OUTPUT
                        PUT8(sbug.addr, sbug.data);
#endif
                    }
                    sbug.addr++;
                    data_count--;
                    state = 4;
                }
                break;
            default:
                state=0;
                break;
        }
    }
    hexstring("Loading program at entry point 0x%08X...\n", entry_start);

    return 0;
}
