#ifndef __MB_EVENT_H_
#define __MB_EVENT_H_

#include "mbcpu.h"
#include "modbus.h"

/* ----------------------- Supporting functions -----------------------------*/
bool xMBEventInit(mb_device_t *dev);
bool xMBEventPost(mb_device_t *dev);
bool xMBEventGet(mb_device_t *dev);

#endif
