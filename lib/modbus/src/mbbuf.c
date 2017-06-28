#include "mbframe.h"
#include "mbproto.h"
#include "mbbuf.h"

#if MB_MASTER_ENABLED > 0
#include "mem_mange.h"

mb_request_t *xMB_ReqBufNew(mb_Mode_t mode,uint16_t Pdusize)
{
    uint16_t size;
    mb_request_t *req;
    
    req = (mb_request_t *)mb_malloc(sizeof(mb_request_t));
    if(req){
        memset(req,0,sizeof(mb_request_t));
        
        if(mode == MB_RTU)
            size = MB_SER_ADU_SIZE_ADDR + MB_SER_ADU_SIZE_CRC;
        else if(mode == MB_ASCII)
            size = MB_SER_ADU_SIZE_ADDR + MB_SER_ADU_SIZE_LRC;
        else
            size = MB_TCP_ADU_SIZE_MBAP;
        
        req->padu = (uint8_t *)mb_malloc(size + Pdusize);
        if(req->padu == NULL){
            mb_free(req);
            req = NULL;
        }
    }
    
    return req;
}

void vMB_ReqBufDelete(void *ptr)
{
    if(ptr){
        mb_free(((mb_request_t *)ptr)->padu);
        mb_free(ptr);
    }
}

/* set head and return head length */
uint8_t xMBsetHead(mb_Mode_t mode, uint8_t *pAdu, uint8_t slaveaddr, uint16_t pdulength)
{
    if(mode == MB_RTU || mode == MB_ASCII){
        pAdu[MB_SER_ADU_ADDR_OFFSET] = slaveaddr;

        /* rtu ascii header size */
        return MB_SER_ADU_SIZE_ADDR;
    }
    else{
        pAdu[MB_TCP_ADU_PID_OFFSET]     = MB_TCP_PROTOCOL_ID >> 8;
        pAdu[MB_TCP_ADU_PID_OFFSET + 1] = MB_TCP_PROTOCOL_ID;
        pAdu[MB_TCP_ADU_LEN_OFFSET]     = (pdulength + 1) >> 8;
        pAdu[MB_TCP_ADU_LEN_OFFSET + 1] = (pdulength + 1) & 0xff;
        pAdu[MB_TCP_ADU_UID_OFFSET]     = slaveaddr;

        //  tcp MBAP header size
        return MB_TCP_ADU_SIZE_MBAP;
    }
}
#endif
