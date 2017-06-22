
#ifndef __MB_BUF_H_
#define __MB_BUF_H_


#include "mbconfig.h"
#include "mbcpu.h"

#include "modbus.h"

mb_request_t *xMB_ReqBufNew(mb_Mode_t mode,uint16_t Pdusize);
void vMB_ReqBufDelete(void *ptr);
uint8_t xMBsetHead(mb_Mode_t mode, uint8_t *pAdu, uint8_t slaveaddr, uint16_t pdulength);


#endif

