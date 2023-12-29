#include <stdio.h>
#include "SdSpi.h"

void uint8ToBin(uint8_t value) {
    printf("0b");
    for (uint32_t k = 8; k != 0; k--) {
        printf("%s", (value & 1 << (k -1)) ? "1" : "0");
    }
    printf("  0x%x\n", value);
}

uint32_t crcCalc(uint8_t pol, uint8_t message[], uint32_t messageSize)
{
    uint32_t polSize = 8;
    uint8_t crc = 0;
    uint32_t bitCnt = 0;
    uint8_t tempByte = 0;
    bool isOne = false;
    printf("messageSize = %u\n", messageSize);

    messageSize++;
    while (messageSize) {
        tempByte = (messageSize == 1) ? 0 : message[messageSize - 2];
        bitCnt = 8;
        printf("byte[%u] = %x\n", messageSize - 1, tempByte);
        while(bitCnt--) {
            if (isOne = crc & 0b100)
            isOne = crc & 0b100;
            crc <<= 1;
            crc += tempByte & 0x80 ? 1 : 0;
            tempByte <<= 1;
            if (isOne) {
                printf("invert:");
                uint8ToBin(crc);
                crc ^= pol;
                printf("       ");
                uint8ToBin(crc);
            } else {
                uint8ToBin(crc);
            }
            crc &= 0b111;
        }
        messageSize--;
    }
    return crc & 0b111;
}

uint32_t crc8Calc(uint8_t pol, uint8_t message[], uint32_t messageSize)
{
    bool invert = false;
    uint8_t crc = 0;
    uint32_t bitCnt = 0;
    uint8_t tempByte = 0;

    uint32_t cnt = 0;
    crc = message[0] >> 1;
    tempByte = message[0] << 7;
    bitCnt = 1;
    printf("next byte:                                         0x%u\n", message[0]);
    while (cnt < messageSize) {
        while(bitCnt--) {
            if (crc & 0x80) {
                printf("crc after[%u]:    ", bitCnt);
                uint8ToBin(crc);
                crc ^= pol;
                //printf("invert:\n");
                invert = true;
            }
            crc <<= 1;
            crc += tempByte >> 7;
            tempByte <<= 1;
            if (invert) {
                printf("crc after[%u]:    ", bitCnt);
                uint8ToBin(crc);
                printf("\n");
                invert = false;
            }
        }
        tempByte = message[++cnt];
        bitCnt = 8;
        printf("next byte:                                         [%u] = 0x%u\n", cnt - 1, tempByte);
    }
    printf("next byte:                                             LAST\n");
    tempByte = 0;
    bitCnt = 8;
    while(bitCnt--) {
        if (crc & 0x80) {
            printf("crc after[%u]:    ", bitCnt);
            uint8ToBin(crc);
            crc ^= pol;
            //printf("invert:\n");
            invert = true;
        }
        crc <<= 1;
        crc += tempByte >> 7;
        tempByte <<= 1;

        if (invert) {
            printf("crc after[%u]:    ", bitCnt);
            uint8ToBin(crc);
            printf("\n");
            invert = false;
        }
    }
    crc >>= 1;
    return crc & 0b1111111;
}

uint32_t calcBitNumber(uint32_t number)
{
    uint32_t bits = 0;

    if (number != 0)
    {
        while((number - 1) & number) {
            number = (number - 1) & number;
            bits++;
        }

        bits++;
    }

    return bits;
}

/**
 * intend - намереваться  
 * consider - учитывать
 * suggested - предлодение
 */

int main(int argCnt, char **argVal){
    uint8_t message1[] = {0b01000000, 0x00, 0x00, 0x00, 0x00}; // crc7 = 0b01001010 = 0x4a
    uint8_t message2[] = {0b01010001, 0x00, 0x00, 0x00, 0x00}; // crc7 = 0b00101010 = 0x2a
    uint8_t message3[] = {0b00010001, 0x00, 0x00, 0x09, 0x00}; // crc7 = 0b00110011 = 0x33
    uint8_t message4[] = {0x17, 0x01, 0x02, 0xff, 0xfe}; // crc7 = 6c
    //uint8_t crc = crc8Calc(0b10001001, message, sizeof(message));
    uint8_t crc;
    //printf("crc7(message1, sizeof(message1)) = %x\n", crc7(message1, sizeof(message1)));
    //printf("crc7(message2, sizeof(message1)) = %x\n", crc7(message2, sizeof(message2)));
    //printf("crc7(message3, sizeof(message1)) = %x\n", crc7(message3, sizeof(message3)));
    //printf("crc7(message4, sizeof(message1)) = %x\n", crc = crc7(message4, sizeof(message4)));
    //uint8ToBin(crc);

    printf("Hellow world");
}