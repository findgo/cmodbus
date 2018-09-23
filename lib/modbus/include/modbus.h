
#ifndef __MODBUS_H_
#define __MODBUS_H_

#include "mb.h"

#if MB_RTU_ENABLED > 0 ||  MB_ASCII_ENABLED > 0
//public
uint32_t MbRegBufSizeCal(     uint16_t reg_holding_num,
                               uint16_t reg_input_num,
                               uint16_t reg_coils_num,
                               uint16_t reg_discrete_num);
uint8_t *MbRegBufNew(uint32_t size);
void MbRegBufFree(void *ptr);


#if MB_MASTER_ENABLED
/* TODO implement modbus master */
MbmDev_t *MbmNew(MbMode_t eMode, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity);
void MbmRemove(uint8_t ucPort);
MbErrorCode_t MbmSetPara(MbmDev_t *dev, 
                                    uint8_t retry,uint32_t replytimeout,
                                    uint32_t delaypolltime, uint32_t broadcastturntime);
MbmNode_t *MbmNodeNew(uint8_t slaveaddr,
                                uint16_t reg_holding_addr_start,
                                uint16_t reg_holding_num,
                                uint16_t reg_input_addr_start,
                                uint16_t reg_input_num,
                                uint16_t reg_coils_addr_start,
                                uint16_t reg_coils_num,
                                uint16_t reg_discrete_addr_start,
                                uint16_t reg_discrete_num);
void MbmNodeFree(MbmNode_t *node);
MbErrorCode_t MbmAddNode(MbmDev_t *dev, MbmNode_t *node);
MbErrorCode_t MbmRemoveNode(MbmDev_t *dev, uint8_t slaveaddr);
MbmNode_t *MbmSearchNode(MbmDev_t *dev,uint8_t slaveaddr);
MbErrorCode_t MbmvStart(MbmDev_t *dev);
MbErrorCode_t MbmStop(MbmDev_t *dev);
MbErrorCode_t MbmClose(MbmDev_t *dev);
void MbmPoll(void);

/* for bits */
/* for request */
MbReqResult_t MbmReqRdCoils(MbmDev_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Coilcnt, uint16_t scanrate);
MbReqResult_t MbmReqWrCoil(MbmDev_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegAddr, uint16_t val);
MbReqResult_t MbmReqWrMulCoils(MbmDev_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Coilcnt,
                                        uint8_t *valbuf, uint16_t valcnt);
MbReqResult_t MbmReqRdDiscreteInputs(MbmDev_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Discnt, uint16_t scanrate);

/* for register */                                    
/* for request */
MbReqResult_t MbmReqRdHoldingRegister(MbmDev_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate);
MbReqResult_t MbmReqWrHoldingRegister(MbmDev_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegAddr, uint16_t val);
MbReqResult_t MbmReqWrMulHoldingRegister(MbmDev_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegStartAddr, uint16_t Regcnt,
                                                uint16_t *valbuf, uint16_t valcnt);
MbReqResult_t MbmReqRdInputRegister(MbmDev_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate);

MbReqResult_t MbmReqRdWrMulHoldingRegister(MbmDev_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegReadStartAddr, uint16_t RegReadCnt,
                                                uint16_t RegWriteStartAddr, uint16_t RegWriteCnt,
                                                uint16_t *valbuf, uint16_t valcnt);

#endif

#if MB_SLAVE_ENABLED > 0

// for slave ,get register start address
#define xMbsRegHoldPtr(pdev) ((uint16_t *)((MbsDev_t *)pdev)->regs.pReghold)
#define xMbsRegInputPtr(pdev) ((uint16_t *)((MbsDev_t *)pdev)->regs.pReginput)
#define xMbsRegCoilPtr(pdev) ((uint8_t *)((MbsDev_t *)pdev)->regs.pRegCoil)
#define xMbsRegDiscPtr(pdev) ((uint8_t *)((MbsDev_t *)pdev)->regs.pRegDisc)


MbErrorCode_t MbsSetSlaveID(MbReg_t *regs, uint8_t ucSlaveID, bool xIsRunning,
                uint8_t const *pucAdditional, uint16_t usAdditionalLen );

MbErrorCode_t MbsRegisterCB(uint8_t ucFunctionCode, pMbsFunctionHandler pxHandler);

MbsDev_t *MbsNew(MbMode_t eMode, uint8_t ucSlaveAddress, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity);

void MbsFree(uint8_t ucPort);
MbErrorCode_t MbsRegAssign(MbsDev_t *dev,
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

MbErrorCode_t MbsStart(MbsDev_t *dev);
MbErrorCode_t MbsStop(MbsDev_t *dev);
MbErrorCode_t MbsClose(MbsDev_t *dev);
void MbsPoll(void);

#endif

#endif

#endif

