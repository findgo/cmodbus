
#ifndef __MODBUS_H_
#define __MODBUS_H_

#include "mb.h"

#if MB_RTU_ENABLED > 0 ||  MB_ASCII_ENABLED > 0
//public
uint32_t xMBRegBufSizeCal(     uint16_t reg_holding_num,
                               uint16_t reg_input_num,
                               uint16_t reg_coils_num,
                               uint16_t reg_discrete_num);
uint8_t *xMBRegBufNew(uint32_t size);
void vMBRegBufFree(void *ptr);


#if MB_MASTER_ENABLED
/* TODO implement modbus master */
Mbm_Device_t *xMBMNew(mb_Mode_t eMode, uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity);
void vMBMFree(uint8_t ucPort);
mb_ErrorCode_t eMBMSetPara(Mbm_Device_t *dev, 
                                    uint8_t retry,uint32_t replytimeout,
                                    uint32_t delaypolltime, uint32_t broadcastturntime);
//mb_ErrorCode_t eMBMNodedelete(Mbm_Device_t *dev, uint8_t slaveaddr);
mbm_slavenode_t *xMBMNodeNew(uint8_t slaveaddr,
                                uint16_t reg_holding_addr_start,
                                uint16_t reg_holding_num,
                                uint16_t reg_input_addr_start,
                                uint16_t reg_input_num,
                                uint16_t reg_coils_addr_start,
                                uint16_t reg_coils_num,
                                uint16_t reg_discrete_addr_start,
                                uint16_t reg_discrete_num);
void vMBMNodeFree(mbm_slavenode_t *node);
mb_ErrorCode_t eMBMNodeadd(Mbm_Device_t *dev, mbm_slavenode_t *node);
mb_ErrorCode_t eMBMNodedelete(Mbm_Device_t *dev, mbm_slavenode_t *node);
mbm_slavenode_t *xMBMNodeSearch(Mbm_Device_t *dev,uint8_t slaveaddr);
mb_ErrorCode_t eMBMStart(Mbm_Device_t *dev);
mb_ErrorCode_t eMBMStop(Mbm_Device_t *dev);
mb_ErrorCode_t eMBMClose(Mbm_Device_t *dev);
void vMBMPoll(void);

/* for bits */
/* for request */
mb_reqresult_t eMBMReqRdCoils(Mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Coilcnt, uint16_t scanrate, pReqResultCB cb);
mb_reqresult_t eMBMReqWrCoil(Mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegAddr, uint16_t val, pReqResultCB cb);
mb_reqresult_t eMBMReqWrMulCoils(Mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Coilcnt,
                                        uint8_t *valbuf, uint16_t valcnt, pReqResultCB cb);
mb_reqresult_t eMBMReqRdDiscreteInputs(Mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Discnt, uint16_t scanrate, pReqResultCB cb);

/* for register */                                    
/* for request */
mb_reqresult_t eMBMReqRdHoldingRegister(Mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate, pReqResultCB cb);
mb_reqresult_t eMBMReqWrHoldingRegister(Mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegAddr, uint16_t val, pReqResultCB cb);
mb_reqresult_t eMBMReqWrMulHoldingRegister(Mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegStartAddr, uint16_t Regcnt,
                                                uint16_t *valbuf, uint16_t valcnt, pReqResultCB cb);
mb_reqresult_t eMBMReqRdInputRegister(Mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate, pReqResultCB cb);

mb_reqresult_t eMBMReqRdWrMulHoldingRegister(Mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegReadStartAddr, uint16_t RegReadCnt,
                                                uint16_t RegWriteStartAddr, uint16_t RegWriteCnt,
                                                uint16_t *valbuf, uint16_t valcnt, pReqResultCB cb);

#endif

#if MB_SLAVE_ENABLED > 0

// for slave ,get register start address
#define xMbsRegHoldPtr(pdev) ((uint16_t *)((Mbs_Device_t *)pdev)->regs.pReghold)
#define xMbsRegInputPtr(pdev) ((uint16_t *)((Mbs_Device_t *)pdev)->regs.pReginput)
#define xMbsRegCoilPtr(pdev) ((uint8_t *)((Mbs_Device_t *)pdev)->regs.pRegCoil)
#define xMbsRegDiscPtr(pdev) ((uint8_t *)((Mbs_Device_t *)pdev)->regs.pRegDisc)


mb_ErrorCode_t eMbsSetSlaveID(Mb_Reg_t *regs, uint8_t ucSlaveID, bool xIsRunning,
                uint8_t const *pucAdditional, uint16_t usAdditionalLen );

mb_ErrorCode_t eMbsRegisterCB(uint8_t ucFunctionCode, pxMbsFunctionHandler pxHandler);

Mbs_Device_t *xMbsNew(mb_Mode_t eMode, uint8_t ucSlaveAddress, uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity);

void vMbsFree(uint8_t ucPort);
mb_ErrorCode_t eMbsRegAssign(Mbs_Device_t *dev,
                                uint8_t *regstoragebuf,  
                                uint32_t regstoragesize, 
                                uint16_t reg_holding_addr_start,
                                uint16_t reg_holding_num,
                                uint16_t reg_input_addr_start,
                                uint16_t reg_input_num,
                                uint16_t reg_coils_addr_start,
                                uint16_t reg_coils_num,
                                uint16_t reg_discrete_addr_start,
                                uint16_t reg_discrete_num);

mb_ErrorCode_t eMbsStart(Mbs_Device_t *dev);
mb_ErrorCode_t eMbsStop(Mbs_Device_t *dev);
mb_ErrorCode_t eMbsClose(Mbs_Device_t *dev);
void vMbsPoll(void);

#endif

#endif

#endif

