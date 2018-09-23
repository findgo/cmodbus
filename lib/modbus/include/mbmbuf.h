
#ifndef __MBM_BUF_H_
#define __MBM_BUF_H_


#include "mbconfig.h"
#include "mbcpu.h"

#include "mb.h"

MbmReq_t *MbmReqBufNew(MbMode_t mode,uint16_t Pdusize);
void MbmReqBufDelete(void *ptr);
uint8_t MbmsetHead(MbMode_t mode, uint8_t *pAdu, uint8_t slaveaddr, uint16_t pdulength);


#endif

