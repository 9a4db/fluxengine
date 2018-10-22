#ifndef CRC_H
#define CRC_H

#define CCITT_POLY 0x1021

extern uint16_t crc16(uint16_t poly, const uint8_t* start, const uint8_t* end);

#endif

