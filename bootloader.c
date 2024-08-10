// This is basically a simualtion in parsing an SREC file 
// that would load your program
#include <stdio.h>
#include <stdlib.h>

FILE *fp;
void init_file() {
    char *file = "notmain.srec";

    fp = fopen(file, "r");
    if (fp == NULL) {
        fprintf(stderr, "error opening file %s\n", file);
        exit(1);
    }
}

#define uart_init() init_file()
#define uart_send(...)  putchar(__VA_ARGS__)
#define hexstring(fmt, ...) printf(fmt, __VA_ARGS__)

int uart_recv() {
    unsigned int ra = fgetc(fp);
    return ra != EOF ? ra : 0;
}

void PUT8(unsigned int address, unsigned char data) {
    printf("Writing %02X to %08X\n", data, address); 
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

int main(int argc, char**argv) 
{ 
    int cnt = 0;
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

    unsigned int entry = 0x8000;
    //while (1) {
    while ((ra = uart_recv())) {
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
                        //hexstring(sbug.type[1]);
                        //state = 0;
                        state++;
                        break;
                    case '3':
                        sbug.type[1] = '3';
                        //hexstring(sbug.type[1]);
                        state++;
                        break;
                    case '7':
                        sbug.type[1] = '7';
                        hexstring("%c\n", sbug.type[1]);
                        hexstring("%08X\n",0xDEADBEEF);
                        hexstring("%X\n", sbug.checksum);
                        //state = 0;
                        state++;
                        break;
                    default:
                        hexstring("%c\n", ra);
                        hexstring("%08X\n", 0xBADBAD01);
                        return 1;
                }
                break;
            case 2:
                sbug.count = ctonib(ra);
                state++;
                break;
            case 3:
                sbug.count <<= 4;
                sbug.count |= ctonib(ra);
                hexstring("-%d-\n",sbug.count);
                sbug.checksum += (sbug.count&0xFF);
                sbug.addr=0;
                data_count = sbug.count*2; // TODO: maybe correct - need to figure out how the data count reaches the end to the check sum but doesnt write to address 
                addr_count = 0;
                hexstring("*%d*\n",data_count);
                printf("%d++\n", sbug.checksum);
                state++;
                break;
            case 4:
                if (addr_count == 8) {
                    sbug.data = ctonib(ra);
                    data_count--;
                    state++;
                    //printf("x%X\n", sbug.addr);
                    printf("================\n");
                } else {
                    sbug.addr <<= 4;
                    sbug.addr |= ctonib(ra);
                    hexstring("BA: x%0X data_count %d\n", ctonib(ra), data_count);
                    // TODO: fix how checksum accumulates. Should accummulate byte by byte but 
                    if (addr_count % 2) { // accumulate checksum on a hexdigit pair basis
                        sbug.checksum += (sbug.addr&0xFF);
                        printf("x%X++ x%X\n", sbug.checksum, sbug.addr&0xFF);
                    }
                    addr_count++;
                    data_count--;
                }
                break;
            case 5:
                if (data_count == 1) {
                    // Indicates end of current line of srec line (i.e we are at the checksum byte)
                    sbug.checksum = 0xFF - (sbug.checksum & 0xFF);
                    hexstring("ADDRESS: x%08X______________x%02X\n", sbug.addr, sbug.checksum&0xFF);
                    state = 0;
                    //data_count = 0;
                    //addr_count = 0;
                    //return 0;
                } else {
                    sbug.data <<= 4;
                    sbug.data |= ctonib(ra);
                    if (data_count > 0) {
                        if (data_count % 2) {
                            sbug.checksum += (sbug.data&0xFF);
                            printf("x%X++ x%X\n", sbug.checksum, sbug.data&0xFF);
                            //cnt++;
                            //if (cnt == 32) { return 0; }
                            i++;
                        }
                        //printf("data_count: %d\n", data_count);
                        //PUT8(sbug.addr, sbug.data);
                    }
                    //PUT32(sbug.addr, sbug.data);
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
    return 0;
}
