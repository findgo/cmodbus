#ifndef __MB_EVENT_H_
#define __MB_EVENT_H_

#include "mbcpu.h"
#include "modbus.h"

/* ----------------------- Supporting functions -----------------------------*/
bool xMBSemBinaryInit(mb_device_t *dev);
bool xMBSemGive(mb_device_t *dev);
bool xMBSemTake(mb_device_t *dev);

#endif
