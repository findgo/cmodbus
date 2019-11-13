#include "port.h"
#include "mb.h"
#include "mbfunc.h"
#include "mbutils.h"

#if MB_RTU_ENABLED > 0

#include "mbrtu.h"

#endif

#if MB_ASCII_ENABLED > 0
#include "mbascii.h"
#endif

#if MB_TCP_ENABLED > 0
#include "mbtcp.h"
#endif

#if (MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED > 0) && MB_SLAVE_ENABLED > 0


// local variate
static MbsDev_t mbsDevTable[MBS_SUPPORT_MULTIPLE_NUMBER];

//local function
static MbErrorCode_t __MbsAduFrameHandle(MbsDev_t *dev);

MbsHandle_t MbsNew(MbMode_t mode, uint8_t slaveID, uint8_t port, uint32_t baudRate, MbParity_t parity) {
    MbsDev_t *dev = NULL;
    MbErrorCode_t status;
    uint8_t i;

    /* check preconditions */
    if ((slaveID == MB_ADDRESS_BROADCAST) || (slaveID < MB_ADDRESS_MIN) || (slaveID > MB_ADDRESS_MAX)) {
        return NULL;
    }

    // search device table and then check port exit and dev in use ?
    for (i = 0; i < MBS_SUPPORT_MULTIPLE_NUMBER; i++) {
        if (mbsDevTable[i].inuse == 0) {
            dev = (MbsDev_t *) &mbsDevTable[i];
            break;
        } else if (mbsDevTable[i].port == port) {
            return NULL; // find port in used
        }
    }
    // no space in device table
    if (i >= MBS_SUPPORT_MULTIPLE_NUMBER)
        return NULL;

    // find space to go on
    memset(dev, 0, sizeof(MbsDev_t));

    switch (mode) {
#if MB_RTU_ENABLED > 0
        case MB_RTU:
            dev->pStartCur = MbsRTUStart;
            dev->pStopCur = MbsRTUStop;
            dev->pCloseCur = MbsRTUClose;
            dev->pSendCur = MbsRTUSend;
            dev->pReceiveParseCur = MbsRTUReceiveParse;

            status = MbsRTUInit(dev, port, baudRate, parity);
            break;
#endif
#if MB_ASCII_ENABLED > 0
        case MB_ASCII:
            dev->pStartCur = MbsASCIIStart;
            dev->pStopCur = MbsASCIIStop;
            dev->pCloseCur = MbsASCIIClose;
            dev->pSendCur = MbsASCIISend;
            dev->pReceiveParseCur = MbsASCIIReceiveParse;

            status = MbsASCIIInit(dev, port, baudRate, parity);
            break;
#endif
        default:
            status = MB_EINVAL;
            break;
    }

    // init failed
    if (status != MB_ENOERR) {
        return NULL;
    }

    dev->inuse = 1;             // mark it in use!
    dev->port = port;
    dev->slaveID = slaveID;  /* set slave address */
    dev->mode = mode;
    dev->state = DEV_STATE_DISABLED;
    dev->eventInFlag = false;

    return (MbsHandle_t) dev;
}

void MbsFree(uint8_t port) {
    uint8_t i;

    for (i = 0; i < MBS_SUPPORT_MULTIPLE_NUMBER; i++) {
        if ((mbsDevTable[i].inuse == 1) && (mbsDevTable[i].port == port)) {
            mbsDevTable[i].inuse = 0; // mark it not in use!
            break;
        }
    }
}


// static uint8_t __aligned(2) regBuf[REG_COILS_SIZE / 8 + REG_DISCRETE_SIZE / 8 + REG_INPUT_NREGS * 2 + REG_HOLDING_NREGS * 2];
MbErrorCode_t MbsRegAssign(MbsHandle_t dev,
                           uint8_t *storageBuf, uint32_t storageSize,
                           uint16_t holdingAddrStart, uint16_t holdingNum,
                           uint16_t inputAddrStart, uint16_t inputNum,
                           uint16_t coilsAddrStart, uint16_t coilsNum,
                           uint16_t discreteAddrStart, uint16_t discreteNum) {
    uint32_t offset;
    MbReg_t *pRegs;

    if (dev == NULL || storageBuf == NULL)
        return MB_EINVAL;

    if (storageSize < MbRegBufSizeCal(holdingNum, inputNum, coilsNum, discreteNum))
        return MB_EINVAL;

    pRegs = (MbReg_t *) &(((MbsDev_t *) dev)->regs);

    pRegs->holdingAddrStart = holdingAddrStart;
    pRegs->holdingNum = holdingNum;
    pRegs->inputAddrStart = inputAddrStart;
    pRegs->inputNum = inputNum;

    pRegs->coilsAddrStart = coilsAddrStart;
    pRegs->coilsNum = coilsNum;
    pRegs->discreteAddrStart = discreteAddrStart;
    pRegs->discreteNum = discreteNum;

    // hold register
    pRegs->pHolding = (uint16_t *) &storageBuf[0];
    offset = holdingNum * sizeof(uint16_t);
    // input register
    pRegs->pInput = (uint16_t *) &storageBuf[offset];
    offset += inputNum * sizeof(uint16_t);
    // coil register
    pRegs->pCoil = &storageBuf[offset];
    offset += (coilsNum >> 3) + (((coilsNum & 0x07) > 0) ? 1 : 0);
    // disc register
    pRegs->pDiscrete = &storageBuf[offset];
    //offset += (discreteNum >> 3) + (((discreteNum & 0x07) > 0) ? 1 : 0);

    return MB_ENOERR;
}

MbErrorCode_t MbsRegAssignSingle(MbsHandle_t dev,
                                 uint16_t *holdingBuff, uint16_t holdingAddrStart, uint16_t holdingNum,
                                 uint16_t *inputBuff, uint16_t inputAddrStart, uint16_t inputNum,
                                 uint8_t *coilsBuff, uint16_t coilsAddrStart, uint16_t coilsNum,
                                 uint8_t *discreteBuff, uint16_t discreteAddrStart, uint16_t discreteNum) {
    MbReg_t *pRegs;

    if (dev == NULL)
        return MB_EINVAL;

    pRegs = (MbReg_t *) &(((MbsDev_t *) dev)->regs);

    pRegs->holdingAddrStart = holdingAddrStart;
    pRegs->holdingNum = holdingNum;
    pRegs->pHolding = holdingBuff;

    pRegs->inputAddrStart = inputAddrStart;
    pRegs->inputNum = inputNum;
    pRegs->pInput = inputBuff;

    pRegs->coilsAddrStart = coilsAddrStart;
    pRegs->coilsNum = coilsNum;
    pRegs->pCoil = coilsBuff;

    pRegs->discreteAddrStart = discreteAddrStart;
    pRegs->discreteNum = discreteNum;
    pRegs->pDiscrete = discreteBuff;

    return MB_ENOERR;
}

MbErrorCode_t MbsStart(MbsHandle_t dev) {
    MbsDev_t *pDev = (MbsDev_t *) dev;

    if (pDev->state == DEV_STATE_NOT_INITIALIZED)
        return MB_EILLSTATE;

    if (pDev->state == DEV_STATE_DISABLED) {
        /* Activate the protocol stack. */
        pDev->pStartCur(dev);
        pDev->state = DEV_STATE_ENABLED;
    }

    return MB_ENOERR;
}

MbErrorCode_t MbsStop(MbsHandle_t dev) {
    MbsDev_t *pDev = (MbsDev_t *) dev;

    if (pDev->state == DEV_STATE_NOT_INITIALIZED)
        return MB_EILLSTATE;

    if (pDev->state == DEV_STATE_ENABLED) {
        pDev->pStopCur(dev);
        pDev->state = DEV_STATE_DISABLED;
    }

    return MB_ENOERR;
}

MbErrorCode_t MbsClose(MbsHandle_t dev) {
    MbsDev_t *pDev = (MbsDev_t *) dev;

    // must be stop first then it can close
    if (pDev->state == DEV_STATE_DISABLED) {
        if (pDev->pCloseCur != NULL) {
            pDev->pCloseCur(dev);
        }

        return MB_ENOERR;
    }

    return MB_EILLSTATE;
}

void MbsPoll(void) {
    uint8_t i;

    for (i = 0; i < MBS_SUPPORT_MULTIPLE_NUMBER; i++) {
        if (mbsDevTable[i].inuse) {
            (void) __MbsAduFrameHandle(&mbsDevTable[i]);
        }
    }
}

static MbErrorCode_t __MbsAduFrameHandle(MbsDev_t *dev) {
    MbsAduFrame_t aduFramePkt;
    MbException_t exception;
    uint8_t rcvAddress;
    pMbsFunctionHandler handle;
    MbErrorCode_t status = MB_ENOERR;

    /* Check if the protocol stack is ready. */
    if (dev->state != DEV_STATE_ENABLED) {
        return MB_EILLSTATE;
    }

    /* Check if there is a event available. If not return control to caller.
     * Otherwise we will handle the event. */
    if (dev->eventInFlag) {
        dev->eventInFlag = false;
        /* parser a receive adu frame */
        status = dev->pReceiveParseCur(dev, &aduFramePkt);
        if (status != MB_ENOERR)
            return status;

        rcvAddress = aduFramePkt.hdr.inRoute.slaveID;

        /* Check if the frame is for us. If not ignore the frame. */
        if ((rcvAddress == dev->slaveID) || (rcvAddress == MB_ADDRESS_BROADCAST)) {
            exception = MB_EX_ILLEGAL_FUNCTION;
            handle = MbsFuncHandleSearch(aduFramePkt.functionCode);
            if (handle) {
                exception = handle(&dev->regs, aduFramePkt.pPduFrame, &aduFramePkt.pduFrameLength);
            }

            /* If the request was not sent to the broadcast address and then we return a reply. */
            if (rcvAddress == MB_ADDRESS_BROADCAST)
                return MB_ENOERR;

            if (exception != MB_EX_NONE) {
                /* An exception occured. Build an error frame. */
                aduFramePkt.pPduFrame = 0;
                aduFramePkt.pPduFrame[aduFramePkt.pduFrameLength++] =
                        (uint8_t) (aduFramePkt.functionCode | MB_FUNC_ERROR);
                aduFramePkt.pPduFrame[aduFramePkt.pduFrameLength++] = exception;
            }

            if ((dev->mode == MB_ASCII) && MBS_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS) {
                MbPortTimersDelay(dev->port, MBS_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS);
            }

            /* send a reply */
            (void) dev->pSendCur(dev, dev->slaveID, aduFramePkt.pPduFrame, aduFramePkt.pduFrameLength);
        }
    }

    return MB_ENOERR;
}

#endif

