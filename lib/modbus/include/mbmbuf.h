
#ifndef __MBM_BUF_H_
#define __MBM_BUF_H_


#include "mbconfig.h"
#include "mbcpu.h"

#include "mb.h"

mbm_request_t *xMBM_ReqBufNew(mb_Mode_t mode,uint16_t Pdusize);
void vMBM_ReqBufDelete(void *ptr);
uint8_t xMBMsetHead(mb_Mode_t mode, uint8_t *pAdu, uint8_t slaveaddr, uint16_t pdulength);


#endif

