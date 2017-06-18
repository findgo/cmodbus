#ifndef __MB_EVENT_H_
#define __MB_EVENT_H_

#include "mbcpu.h"
#include "modbus.h"

/* ----------------------- Supporting functions -----------------------------*/
bool xMBSemBinaryInit(mb_Device_t *dev);
bool xMBSemGive(mb_Device_t *dev);
bool xMBSemTake(mb_Device_t *dev);

#endif
