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
static MbsDev_t mbs_devTal[MBS_SUPPORT_MULTIPLE_NUMBER];

//local function
static MbErrorCode_t __MbsAduFrameHandle(MbsDev_t *dev);

Mbshandle_t MbsNew(MbMode_t mode, uint8_t slaveID, uint8_t port, uint32_t baudRate, MbParity_t parity) {
    MbsDev_t *dev = NULL;
    MbErrorCode_t status;
    uint8_t i;

    /* check preconditions */
    if ((slaveID == MB_ADDRESS_BROADCAST) || (slaveID < MB_ADDRESS_MIN) || (slaveID > MB_ADDRESS_MAX)) {
        return NULL;
    }

    // search device table and then check port exit and dev in use ?
    for (i = 0; i < MBS_SUPPORT_MULTIPLE_NUMBER; i++) {
        if (mbs_devTal[i].inuse == 0) {
            dev = (MbsDev_t *) &mbs_devTal[i];
            break;
        } else if (mbs_devTal[i].port == port) {
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

    return (Mbshandle_t) dev;
}

void MbsFree(uint8_t ucPort) {
    uint8_t i;

    for (i = 0; i < MBS_SUPPORT_MULTIPLE_NUMBER; i++) {
        if ((mbs_devTal[i].inuse == 1) && (mbs_devTal[i].port == ucPort)) {
            mbs_devTal[i].inuse = 0; // mark it not in use!
            break;
        }
    }
}


// static uint8_t __aligned(2) regbuf[REG_COILS_SIZE / 8 + REG_DISCRETE_SIZE / 8 + REG_INPUT_NREGS * 2 + REG_HOLDING_NREGS * 2];
MbErrorCode_t MbsRegAssign(Mbshandle_t dev,
                           uint8_t *regStorageBuf, uint32_t regStorageSize,
                           uint16_t regHoldingAddrStart, uint16_t regHoldingNum,
                           uint16_t regInputAddrStart, uint16_t regInputNum,
                           uint16_t regCoilsAddrStart, uint16_t regCoilsNum,
                           uint16_t regDiscreteAddrStart, uint16_t regDiscreteNum) {
    uint32_t offset;
    MbReg_t *regs;

    if (dev == NULL || regStorageBuf == NULL)
        return MB_EINVAL;

    if (regStorageSize < MbRegBufSizeCal(regHoldingNum, regInputNum, regCoilsNum, regDiscreteNum))
        return MB_EINVAL;

    regs = (MbReg_t *) &(((MbsDev_t *) dev)->regs);

    regs->holdingAddrStart = regHoldingAddrStart;
    regs->holdingNum = regHoldingNum;
    regs->inputAddrStart = regInputAddrStart;
    regs->inputNum = regInputNum;

    regs->coilsAddrStart = regCoilsAddrStart;
    regs->coilsNum = regCoilsNum;
    regs->discreteAddrStart = regDiscreteAddrStart;
    regs->discreteNum = regDiscreteNum;

    // hold register
    regs->pHolding = (uint16_t *) &regStorageBuf[0];
    offset = regHoldingNum * sizeof(uint16_t);
    // input register
    regs->pInput = (uint16_t *) &regStorageBuf[offset];
    offset += regInputNum * sizeof(uint16_t);
    // coil register
    regs->pCoil = &regStorageBuf[offset];
    offset += (regCoilsNum >> 3) + (((regCoilsNum & 0x07) > 0) ? 1 : 0);
    // disc register
    regs->pDiscrete = &regStorageBuf[offset];
    //offset += (regDiscreteNum >> 3) + (((regDiscreteNum & 0x07) > 0) ? 1 : 0);

    return MB_ENOERR;
}

MbErrorCode_t MbsRegAssignSingle(Mbshandle_t dev,
                                 uint16_t *reg_holdingbuf, uint16_t reg_holding_addr_start, uint16_t reg_holding_num,
                                 uint16_t *reg_inputbuf, uint16_t reg_input_addr_start, uint16_t reg_input_num,
                                 uint8_t *reg_coilsbuf, uint16_t reg_coils_addr_start, uint16_t reg_coils_num,
                                 uint8_t *reg_discretebuf, uint16_t reg_discrete_addr_start,
                                 uint16_t reg_discrete_num) {
    MbReg_t *regs;

    if (dev == NULL)
        return MB_EINVAL;

    regs = (MbReg_t *) &(((MbsDev_t *) dev)->regs);

    regs->holdingAddrStart = reg_holding_addr_start;
    regs->holdingNum = reg_holding_num;
    regs->pHolding = reg_holdingbuf;

    regs->inputAddrStart = reg_input_addr_start;
    regs->inputNum = reg_input_num;
    regs->pInput = reg_inputbuf;

    regs->coilsAddrStart = reg_coils_addr_start;
    regs->coilsNum = reg_coils_num;
    regs->pCoil = reg_coilsbuf;

    regs->discreteAddrStart = reg_discrete_addr_start;
    regs->discreteNum = reg_discrete_num;
    regs->pDiscrete = reg_discretebuf;

    return MB_ENOERR;
}

MbErrorCode_t MbsStart(Mbshandle_t dev) {
    MbsDev_t *pdev = (MbsDev_t *) dev;

    if (pdev->state == DEV_STATE_NOT_INITIALIZED)
        return MB_EILLSTATE;

    if (pdev->state == DEV_STATE_DISABLED) {
        /* Activate the protocol stack. */
        pdev->pStartCur(dev);
        pdev->state = DEV_STATE_ENABLED;
    }

    return MB_ENOERR;
}

MbErrorCode_t MbsStop(Mbshandle_t dev) {
    MbsDev_t *pdev = (MbsDev_t *) dev;

    if (pdev->state == DEV_STATE_NOT_INITIALIZED)
        return MB_EILLSTATE;

    if (pdev->state == DEV_STATE_ENABLED) {
        pdev->pStopCur(dev);
        pdev->state = DEV_STATE_DISABLED;
    }

    return MB_ENOERR;
}

MbErrorCode_t MbsClose(Mbshandle_t dev) {
    MbsDev_t *pdev = (MbsDev_t *) dev;

    // must be stop first then it can close
    if (pdev->state == DEV_STATE_DISABLED) {
        if (pdev->pCloseCur != NULL) {
            pdev->pCloseCur(dev);
        }

        return MB_ENOERR;
    }

    return MB_EILLSTATE;
}

void MbsPoll(void) {
    uint8_t i;

    for (i = 0; i < MBS_SUPPORT_MULTIPLE_NUMBER; i++) {
        if (mbs_devTal[i].inuse) {
            (void) __MbsAduFrameHandle(&mbs_devTal[i]);
        }
    }
}

static MbErrorCode_t __MbsAduFrameHandle(MbsDev_t *dev) {
    MbsAduFrame_t aduFramePkt;
    MbException_t eException;
    uint8_t RcvAddress;
    pMbsFunctionHandler handle;
    MbErrorCode_t eStatus = MB_ENOERR;

    /* Check if the protocol stack is ready. */
    if (dev->state != DEV_STATE_ENABLED) {
        return MB_EILLSTATE;
    }

    /* Check if there is a event available. If not return control to caller.
     * Otherwise we will handle the event. */
    if (dev->eventInFlag) {
        dev->eventInFlag = false;
        /* parser a receive adu frame */
        eStatus = dev->pReceiveParseCur(dev, &aduFramePkt);
        if (eStatus != MB_ENOERR)
            return eStatus;

        RcvAddress = aduFramePkt.hdr.introute.slaveID;

        /* Check if the frame is for us. If not ignore the frame. */
        if ((RcvAddress == dev->slaveID) || (RcvAddress == MB_ADDRESS_BROADCAST)) {
            eException = MB_EX_ILLEGAL_FUNCTION;
            handle = MbsFuncHandleSearch(aduFramePkt.FunctionCode);
            if (handle)
                eException = handle(&dev->regs, aduFramePkt.pPduFrame, &aduFramePkt.pduFrameLength);

            /* If the request was not sent to the broadcast address and then we return a reply. */
            if (RcvAddress == MB_ADDRESS_BROADCAST)
                return MB_ENOERR;

            if (eException != MB_EX_NONE) {
                /* An exception occured. Build an error frame. */
                aduFramePkt.pPduFrame = 0;
                aduFramePkt.pPduFrame[aduFramePkt.pduFrameLength++] = (uint8_t) (aduFramePkt.FunctionCode |
                                                                                 MB_FUNC_ERROR);
                aduFramePkt.pPduFrame[aduFramePkt.pduFrameLength++] = eException;
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

