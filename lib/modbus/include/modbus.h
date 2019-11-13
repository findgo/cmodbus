
#ifndef __MODBUS_H_
#define __MODBUS_H_

#include "mb.h"


#if MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED > 0

//public
uint32_t MbRegBufSizeCal(uint16_t holdingNum,
                         uint16_t inputNum,
                         uint16_t coilsNum,
                         uint16_t discreteNum);

#define MbGetRegsHoldPtr(pReg)       ((uint16_t *)(((MbReg_t *)(pReg))->pReghold))
#define MbGetRegsInputPtr(pReg)      ((uint16_t *)(((MbReg_t *)(pReg))->pReginput))
#define MbGetRegsCoilPtr(pReg)       ((uint8_t *)(((MbReg_t *)(pReg))->pRegCoil))
#define MbGetRegsDiscPtr(pReg)       ((uint8_t *)(((MbReg_t *)(pReg))->pRegDisc))

#define MbGetRegsHoldNum(pReg)       (((MbReg_t *)(pReg))->reg_holding_num)
#define MbGetRegsInputNum(pReg)      (((MbReg_t *)(pReg))->reg_input_num)
#define MbGetRegsCoilNum(pReg)       (((MbReg_t *)(pReg))->reg_coils_num)
#define MbGetRegsDiscNum(pReg)       (((MbReg_t *)(pReg))->reg_discrete_num)

#if MB_MASTER_ENABLED

MbErrorCode_t MbmRegisterParseHandleCB(uint8_t functionCode, pMbmParseRspHandler pHandler);

/* TODO implement modbus master */
MbmHandle_t MbmNew(MbMode_t eMode, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity);

void MbmFree(uint8_t ucPort);

MbErrorCode_t MbmSetPara(MbmHandle_t dev, uint8_t retry, uint32_t replytimeout,
                         uint32_t delaypolltime, uint32_t broadcastturntime);

MbmNode_t *MbmNodeNew(uint8_t slaveID,
                      uint16_t holdingAddrStart, uint16_t holdingNum,
                      uint16_t inputAddrStart, uint16_t inputNum,
                      uint16_t coilsAddrStart, uint16_t coilsNum,
                      uint16_t discreteAddrStart, uint16_t discreteNum);

void MbmNodeFree(MbmNode_t *node);

void MbmNodeCallBackAssign(MbmNode_t *node, pfnReqResultCB cb, void *arg);

MbErrorCode_t MbmAddNode(MbmHandle_t dev, MbmNode_t *node);

MbErrorCode_t MbmRemoveNode(MbmHandle_t dev, uint8_t slaveID);

MbmNode_t *MbmSearchNode(MbmHandle_t dev, uint8_t slaveID);

MbErrorCode_t MbmStart(MbmHandle_t dev);

MbErrorCode_t MbmStop(MbmHandle_t dev);

MbErrorCode_t MbmClose(MbmHandle_t dev);

void MbmPoll(void);

/* for bits */
/* for request */
MbReqResult_t MbmReqRdCoils(MbmHandle_t dev, uint8_t slaveID,
                            uint16_t RegStartAddr, uint16_t Coilcnt, uint16_t scanrate);

MbReqResult_t MbmReqWrCoil(MbmHandle_t dev, uint8_t slaveID,
                           uint16_t RegAddr, uint16_t val);

MbReqResult_t MbmReqWrMulCoils(MbmHandle_t dev, uint8_t slaveID,
                               uint16_t RegStartAddr, uint16_t Coilcnt,
                               uint8_t *valbuf, uint16_t valcnt);

MbReqResult_t MbmReqRdDiscreteInputs(MbmHandle_t dev, uint8_t slaveID,
                                     uint16_t RegStartAddr, uint16_t Discnt, uint16_t scanrate);

/* for register */
/* for request */
MbReqResult_t MbmReqRdHoldingRegister(MbmHandle_t dev, uint8_t slaveID,
                                      uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate);

MbReqResult_t MbmReqWrHoldingRegister(MbmHandle_t dev, uint8_t slaveID,
                                      uint16_t RegAddr, uint16_t val);

MbReqResult_t MbmReqWrMulHoldingRegister(MbmHandle_t dev, uint8_t slaveID,
                                         uint16_t RegStartAddr, uint16_t Regcnt,
                                         uint16_t *valbuf, uint16_t valcnt);

MbReqResult_t MbmReqRdInputRegister(MbmHandle_t dev, uint8_t slaveID,
                                    uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate);

MbReqResult_t MbmReqRdWrMulHoldingRegister(MbmHandle_t dev, uint8_t slaveID,
                                           uint16_t RegReadStartAddr, uint16_t RegReadCnt,
                                           uint16_t RegWriteStartAddr, uint16_t RegWriteCnt,
                                           uint16_t *valbuf, uint16_t valcnt);

#define MbmGetReqSlaveID(pReq)  (((MbmReq_t *)(pReq))->slaveID)
#define MbmGetReqFunCode(pReq)  (((MbmReq_t *)(pReq))->funcode)
#define MbmGetReqErrorCnt(pReq) (((MbmReq_t *)(pReq))->errcnt)
#define MbmGetReqRegAddr(pReq)  (((MbmReq_t *)(pReq))->regaddr)
#define MbmGetReqRegCnt(pReq)   (((MbmReq_t *)(pReq))->regcnt)
#define MbmGetNodePtr(pReq)     (((MbmReq_t *)(pReq))->node)
#define MbmGetArgPtr(pReq)      (((MbmReq_t *)(pReq))->node->arg)

#define MbmGetSlaveID(pNode)    (((MbmNode_t *)(pNode))->slaveID)
#define MbmGetRegsPtr(pNode)    (&(((MbmNode_t *)(pNode))->regs))

#endif

#if MB_SLAVE_ENABLED > 0
// for slave, get register pointer
#define MbsGetRegsPtr(dev)  (&(((MbsDev_t *)(dev))->regs))
// for slave ,get register start address
#define MbsGetRegsHoldPtr(dev)      ((uint16_t *)(((MbsDev_t *)(dev))->regs.pReghold))
#define MbsGetRegsInputPtr(dev)     ((uint16_t *)(((MbsDev_t *)(dev))->regs.pReginput))
#define MbsGetRegsCoilPtr(dev)      ((uint8_t *)(((MbsDev_t *)(dev))->regs.pRegCoil))
#define MbsGetRegsDiscPtr(dev)      ((uint8_t *)(((MbsDev_t *)(dev))->regs.pRegDisc))
// for slave ,get register number
#define MbsGetRegsHoldNum(dev)      (((MbsDev_t *)(dev))->regs.reg_holding_num)
#define MbsGetRegsInputNum(dev)     (((MbsDev_t *)(dev))->regs.reg_input_num)
#define MbsGetRegsCoilNum(dev)      (((MbsDev_t *)(dev))->regs.reg_coils_num)
#define MbsGetRegsDiscNum(dev)      (((MbsDev_t *)(dev))->regs.reg_discrete_num)


MbErrorCode_t MbsSetSlaveID(MbReg_t *pRegs, uint8_t slaveID, uint8_t isRunning,
                            uint8_t const *pAdditional, uint16_t additionalLen);

/*********************************************************************
 * @brief   register function code handle
 * @param   functionCode - 功能码
 * @param   pxHandler - 功能码对应的回调函数, NULL: 为注销对应功能码回调
 * @return  
 */
MbErrorCode_t MbsRegisterHandleCB(uint8_t functionCode, pMbsFunctionHandler pHandler);

/*********************************************************************
 * @brief   create new slave modbus device  
 * @param   mode - MB_RTU or MB_ASCII
 * @param   slaveID - slave id
 * @param   port - use which usart port
 * @param   baudRate - bandrate
 * @param   parity - Parity used for characters in serial mode
 * @param   Mbshandle_t - slave device handle ,if failed return NULL 句柄
 * @return  
 */
MbsHandle_t MbsNew(MbMode_t mode, uint8_t slaveID, uint8_t port, uint32_t baudRate, MbParity_t parity);

/*********************************************************************
 * @brief   free the slave modbus device with port
 * @param   ucPort - use which usart port 
 * @return  
 */
void MbsFree(uint8_t ucPort);

/*********************************************************************
 * @brief   assign buffer for register
 * @return  
 */
MbErrorCode_t MbsRegAssign(MbsHandle_t dev,
                           uint8_t *storageBuf, uint32_t storageSize,
                           uint16_t holdingAddrStart, uint16_t holdingNum,
                           uint16_t inputAddrStart, uint16_t inputNum,
                           uint16_t coilsAddrStart, uint16_t coilsNum,
                           uint16_t discreteAddrStart, uint16_t discreteNum);

MbErrorCode_t MbsRegAssignSingle(MbsHandle_t dev,
                                 uint16_t *holdingBuff, uint16_t holdingAddrStart, uint16_t holdingNum,
                                 uint16_t *inputBuff, uint16_t inputAddrStart, uint16_t inputNum,
                                 uint8_t *coilsBuff, uint16_t coilsAddrStart, uint16_t coilsNum,
                                 uint8_t *discreteBuff, uint16_t discreteAddrStart, uint16_t discreteNum);

/*********************************************************************
 * @brief   start the slave device
 * @param   dev - the handle of the slave device
 * @return  
 */
MbErrorCode_t MbsStart(MbsHandle_t dev);

/*********************************************************************
 * @brief   stop the slave device
 * @param   dev - the handle of the slave device
 * @return  
 */
MbErrorCode_t MbsStop(MbsHandle_t dev);

/*********************************************************************
 * @brief   close the slave device, before it,muse be use MbsStop first
 * @param   dev - the handle of the slave device
 * @return  
 */
MbErrorCode_t MbsClose(MbsHandle_t dev);

//
void MbsPoll(void);

#endif

#endif

#endif

