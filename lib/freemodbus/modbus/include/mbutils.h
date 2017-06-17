

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

eMBException prveMBError2Exception( eMBErrorCode eErrorCode );

#ifdef __cplusplus
}
#endif

#endif

