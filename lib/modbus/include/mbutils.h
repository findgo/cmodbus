

#ifndef __MB_UTILS_H
#define __MB_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "mbproto.h"
#include "mbconfig.h"
#include "mbcpu.h"

#include "mb.h"

void MbSetBits(uint8_t *byteBuf, uint16_t bitOffset, uint8_t nBits, uint8_t value);

uint8_t MbGetBits(uint8_t *byteBuf, uint16_t bitOffset, uint8_t nBits);

uint16_t MbCRC16(uint8_t *pFrame, uint16_t len);

uint8_t MbChar2Bin(uint8_t ucCharacter);

uint8_t MbBin2Char(uint8_t ucByte);

uint8_t MbLRC(uint8_t *pucFrame, uint16_t usLen);

MbException_t MbError2Exception(MbErrorCode_t eErrorCode);

const char *MbError2Str(MbException_t exCode);


#ifdef __cplusplus
}
#endif

#endif

