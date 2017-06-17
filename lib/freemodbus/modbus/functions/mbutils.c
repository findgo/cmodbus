
#include "mbutils.h"

eMBException prveMBError2Exception(eMBErrorCode eErrorCode)
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
