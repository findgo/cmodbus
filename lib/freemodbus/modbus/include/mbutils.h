

#ifndef __MB_UTILS_H
#define __MB_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbconfig.h"
#include "mbproto.h"
#include "mbframe.h"
#include "mbcpu.h"

#include "modbus.h"

eMBException_t prveMBError2Exception( mb_ErrorCode_t eErrorCode );
const char *xMBstr2Error(eMBException_t excode);

#ifdef __cplusplus
}
#endif

#endif

