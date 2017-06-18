
#include "mbutils.h"

eMBException_t prveMBError2Exception(mb_ErrorCode_t eErrorCode)
{
    switch ( eErrorCode ){
        case MB_ENOERR:
            return MB_EX_NONE;

        case MB_ENOREG:
            return MB_EX_ILLEGAL_DATA_ADDRESS;

        case MB_ETIMEDOUT:
            return MB_EX_SLAVE_BUSY;

        default:
            return MB_EX_SLAVE_DEVICE_FAILURE;
    }
}

const char *xMBstr2Error(eMBException_t excode)
{
    switch (excode){
        case MB_EX_NONE:
            return "No error!";
        case MB_EX_ILLEGAL_FUNCTION:
            return "Illegal data function!";
        case MB_EX_ILLEGAL_DATA_ADDRESS:
            return "Illegal data address!";
        case MB_EX_ILLEGAL_DATA_VALUE:
            return "Illegal data value!";
        case MB_EX_SLAVE_DEVICE_FAILURE:
            return "Slave device failure!";
        case MB_EX_ACKNOWLEDGE:
            return "Acknowledge";
        case MB_EX_SLAVE_BUSY:
            return "Salve busy";
        case MB_EX_MEMORY_PARITY_ERROR:
            return "Memory parity error!";
        case MB_EX_GATEWAY_PATH_FAILED:
            return "Gateway path failed!";
        case MB_EX_GATEWAY_TGT_FAILED:
            return "Target response failed and gateway generate!";
        default:
            return "Reserve exception code";
    }
}
