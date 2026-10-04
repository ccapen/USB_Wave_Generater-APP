#include "crc.h"

/******************************************************************************
 * Name:    CRC-16/CCITT-FALSE   x16+x12+x5+1
 * Poly:    0x1021
 * Init:    0xFFFF
 * Refin:   False
 * Refout:  False
 * Xorout:  0x0000
 * Note:
 *****************************************************************************/
uint16_t crc16_ccitt_false(uint8_t *data, uint16_t length, uint16_t init)
{
    uint8_t i;
    uint16_t crc = init;        //Initial value
    while(length--)
    {
        crc ^= (uint16_t)(*data++) << 8; // crc ^= (uint6_t)(*data)<<8; data++;
        for (i = 0; i < 8; ++i)
        {
            if ( crc & 0x8000 )
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}
