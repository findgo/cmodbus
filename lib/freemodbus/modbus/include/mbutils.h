

#ifndef _MB_UTILS_H
#define _MB_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbconfig.h"
#include "mbproto.h"
#include "mbframe.h"
#include "mbcpu.h"

#include "modbus.h"

uint16_t usMBCRC16( uint8_t * pucFrame, uint16_t usLen );
eMBException prveMBError2Exception( eMBErrorCode eErrorCode );

#ifdef __cplusplus
}
#endif

#endif

