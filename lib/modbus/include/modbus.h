
#ifndef __MODBUS_H_
#define __MODBUS_H_

#include "mb.h"

#if MB_RTU_ENABLED > 0 ||  MB_ASCII_ENABLED > 0

#if MB_SLAVE_ENABLED > 0

// for slave 
#define xMBRegHoldPtr(pdev) ((uint16_t *)((mbs_Device_t *)dev)->regs.pReghold)
#define xMBRegInputPtr(pdev) ((uint16_t *)((mbs_Device_t *)dev)->regs.pReginput)
#define xMBRegCoilPtr(pdev) ((uint8_t *)((mbs_Device_t *)dev)->regs.pRegCoil)
#define xMBRegDiscPtr(pdev) ((uint8_t *)((mbs_Device_t *)dev)->regs.pRegDisc)
/*! \ingroup modbus
 * \brief Configure the slave id of the device.
 *
 * This function should be called when the Modbus function <em>Report Slave ID</em>
 * is enabled ( By defining MBS_FUNC_OTHER_REP_SLAVEID_ENABLED in mbconfig.h ).
 *
 * \param ucSlaveID Values is returned in the <em>Slave ID</em> byte of the
 *   <em>Report Slave ID</em> response.
 * \param xIsRunning If true the <em>Run Indicator Status</em> byte is set to 0xFF.
 *   otherwise the <em>Run Indicator Status</em> is 0x00.
 * \param pucAdditional Values which should be returned in the <em>Additional</em>
 *   bytes of the <em> Report Slave ID</em> response.
 * \param usAdditionalLen Length of the buffer <code>pucAdditonal</code>.
 *
 * \return If the static buffer defined by MBS_FUNC_OTHER_REP_SLAVEID_BUF in
 *   mbconfig.h is to small it returns mb_ErrorCode_t::MB_ENORES. Otherwise
 *   it returns mb_ErrorCode_t::MB_ENOERR.
 */
mb_ErrorCode_t eMbsSetSlaveID(mb_Reg_t *regs, uint8_t ucSlaveID, bool xIsRunning,
                uint8_t const *pucAdditional, uint16_t usAdditionalLen );

/*! \ingroup modbus
 * \brief Registers a callback handler for a given function code.
 *
 * This function registers a new callback handler for a given function code.
 * The callback handler supplied is responsible for interpreting the Modbus PDU and
 * the creation of an appropriate response. In case of an error it should return
 * one of the possible Modbus exceptions which results in a Modbus exception frame
 * sent by the protocol stack. 
 *
 * \param ucFunctionCode The Modbus function code for which this handler should
 *   be registers. Valid function codes are in the range 1 to 127.
 * \param pxHandler The function handler which should be called in case
 *   such a frame is received. If \c NULL a previously registered function handler
 *   for this function code is removed.
 *
 * \return mb_ErrorCode_t::MB_ENOERR if the handler has been installed. If no
 *   more resources are available it returns mb_ErrorCode_t::MB_ENORES. In this
 *   case the values in mbconfig.h should be adjusted. If the argument was not
 *   valid it returns mb_ErrorCode_t::MB_EINVAL.
 */
mb_ErrorCode_t eMbsRegisterCB(uint8_t ucFunctionCode, pxMBFunctionHandler pxHandler);

mbs_Device_t *xMbsNew(mb_Mode_t eMode, uint8_t ucSlaveAddress, 
                        uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity);

void vMbsDelete(uint8_t ucSlaveAddress);
mb_ErrorCode_t eMbsRegAssign(mbs_Device_t *dev,
                                uint8_t *regbuf,
                                uint32_t regbufsize, 
                                uint16_t reg_holding_addr_start,
                                uint16_t reg_holding_num,
                                uint16_t reg_input_addr_start,
                                uint16_t reg_input_num,
                                uint16_t reg_coils_addr_start,
                                uint16_t reg_coils_num,
                                uint16_t reg_discrete_addr_start,
                                uint16_t reg_discrete_num);
mb_ErrorCode_t eMbsStart(mbs_Device_t *dev);
mb_ErrorCode_t eMbsStop(mbs_Device_t *dev);
mb_ErrorCode_t eMbsClose(mbs_Device_t *dev);
void vMbsPoll(void);
#endif

#if MB_MASTER_ENABLED
/* TODO implement modbus master */
mbm_Device_t *xMBMNew(mb_Mode_t eMode, uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity);
mb_ErrorCode_t eMBMDelete(uint8_t ucPort);
mb_ErrorCode_t eMBMSetPara(mbm_Device_t *dev, 
                                    uint8_t retry,uint32_t replytimeout,
                                    uint32_t delaypolltime, uint32_t broadcastturntime);
mbm_slavenode_t *xMBMNodeNew(mbm_Device_t *dev,
                                        uint8_t slaveaddr,
                                        uint16_t reg_holding_addr_start,
                                        uint16_t reg_holding_num,
                                        uint16_t reg_input_addr_start,
                                        uint16_t reg_input_num,
                                        uint16_t reg_coils_addr_start,
                                        uint16_t reg_coils_num,
                                        uint16_t reg_discrete_addr_start,
                                        uint16_t reg_discrete_num);
mb_ErrorCode_t eMBMNodedelete(mbm_Device_t *dev, uint8_t slaveaddr);
mbm_slavenode_t *xMBMNodeCreate(uint8_t slaveaddr,
                                            uint16_t reg_holding_addr_start,
                                            uint16_t reg_holding_num,
                                            uint16_t reg_input_addr_start,
                                            uint16_t reg_input_num,
                                            uint16_t reg_coils_addr_start,
                                            uint16_t reg_coils_num,
                                            uint16_t reg_discrete_addr_start,
                                            uint16_t reg_discrete_num);
void vMBMNodeDestroy(mbm_slavenode_t *node);
mb_ErrorCode_t eMBMNodeadd(mbm_Device_t *dev, mbm_slavenode_t *node);
mb_ErrorCode_t eMBMNoderemove(mbm_Device_t *dev, mbm_slavenode_t *node);
mbm_slavenode_t *xMBMNodeSearch(mbm_Device_t *dev,uint8_t slaveaddr);
mb_ErrorCode_t eMBMStart(mbm_Device_t *dev);
mb_ErrorCode_t eMBMStop(mbm_Device_t *dev);
mb_ErrorCode_t eMBMClose(mbm_Device_t *dev);
void vMBMPoll(void);

/* for bits */
/* for request */
mb_reqresult_t eMBMReqRdCoils(mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Coilcnt, uint16_t scanrate, pReqResultCB cb);
mb_reqresult_t eMBMReqWrCoil(mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegAddr, uint16_t val, pReqResultCB cb);
mb_reqresult_t eMBMReqWrMulCoils(mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Coilcnt,
                                        uint8_t *valbuf, uint16_t valcnt, pReqResultCB cb);
mb_reqresult_t eMBMReqRdDiscreteInputs(mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Discnt, uint16_t scanrate, pReqResultCB cb);

/* for register */                                    
/* for request */
mb_reqresult_t eMBMReqRdHoldingRegister(mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate, pReqResultCB cb);
mb_reqresult_t eMBMReqWrHoldingRegister(mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegAddr, uint16_t val, pReqResultCB cb);
mb_reqresult_t eMBMReqWrMulHoldingRegister(mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegStartAddr, uint16_t Regcnt,
                                                uint16_t *valbuf, uint16_t valcnt, pReqResultCB cb);
mb_reqresult_t eMBMReqRdInputRegister(mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate, pReqResultCB cb);

mb_reqresult_t eMBMReqRdWrMulHoldingRegister(mbm_Device_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegReadStartAddr, uint16_t RegReadCnt,
                                                uint16_t RegWriteStartAddr, uint16_t RegWriteCnt,
                                                uint16_t *valbuf, uint16_t valcnt, pReqResultCB cb);

#endif

#endif

#endif

