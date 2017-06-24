

#ifndef __MB_UTILS_H
#define __MB_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbconfig.h"
#include "mbcpu.h"

#include "modbus.h"


void vMBSetBits( uint8_t *ucByteBuf, uint16_t usBitOffset, uint8_t ucNBits, uint8_t ucValue );
uint8_t xMBGetBits( uint8_t * ucByteBuf, uint16_t usBitOffset, uint8_t ucNBits );

uint16_t prvxMBCRC16(uint8_t *pucFrame, uint16_t usLen);

uint8_t prvxMBCHAR2BIN(uint8_t ucCharacter);
uint8_t prvxMBBIN2CHAR(uint8_t ucByte);
uint8_t prvxMBLRC(uint8_t *pucFrame, uint16_t usLen);

eMBException_t prveMBError2Exception(mb_ErrorCode_t eErrorCode);
const char *xMBstr2Error(eMBException_t excode);

void *pvMBmemcpy(uint8_t *dst, const uint8_t *src, uint16_t length);
uint32_t xMBRegBufSizeCal(     uint16_t reg_holding_num,
                                  uint16_t reg_input_num,
                                  uint16_t reg_coils_num,
                                  uint16_t reg_discrete_num);

#ifdef __cplusplus
}
#endif

#endif

