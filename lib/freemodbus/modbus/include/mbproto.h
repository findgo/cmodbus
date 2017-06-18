
#ifndef __MB_PROTO_H
#define __MB_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------- Defines ------------------------------------------*/
#define MB_ADDRESS_BROADCAST    ( 0 )   /*! Modbus broadcast address. */
#define MB_ADDRESS_MIN          ( 1 )   /*! Smallest possible slave address. */
#define MB_ADDRESS_MAX          ( 247 ) /*! Biggest possible slave address. */
/* 
 * Modbus TCP does not use any addresses. Fake the source address such 
 * that the processing part deals with this frame. 
 */
#define MB_TCP_PSEUDO_ADDRESS   ( 255 )

/* modbus function code */
#define MB_FUNC_MIN                           ( 1 )
#define MB_FUNC_MAX                           ( 127 )
#define MB_FUNC_NONE                          (  0 )
#define MB_FUNC_READ_COILS                    (  1 )
#define MB_FUNC_READ_DISCRETE_INPUTS          (  2 )
#define MB_FUNC_WRITE_SINGLE_COIL             (  5 )
#define MB_FUNC_WRITE_MULTIPLE_COILS          ( 15 )
#define MB_FUNC_READ_HOLDING_REGISTER         (  3 )
#define MB_FUNC_READ_INPUT_REGISTER           (  4 )
#define MB_FUNC_WRITE_REGISTER                (  6 )
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS      ( 16 )
#define MB_FUNC_READWRITE_MULTIPLE_REGISTERS  ( 23 )
#define MB_FUNC_DIAG_READ_EXCEPTION           (  7 )
#define MB_FUNC_DIAG_DIAGNOSTIC               (  8 )
#define MB_FUNC_DIAG_GET_COM_EVENT_CNT        ( 11 )
#define MB_FUNC_DIAG_GET_COM_EVENT_LOG        ( 12 )
#define MB_FUNC_OTHER_REPORT_SLAVEID          ( 17 )
#define MB_FUNC_ERROR                         ( 128 )

// proto coils disc holding input limit
#define MB_READBITS_CNT_MIN             ( 0x0001 )
#define MB_READBITS_CNT_MAX             ( 0x07D0 )
#define MB_WRITEBITS_CNT_MIN            ( 0x0001 )
#define MB_WRITEBITS_CNT_MAX            ( 0x07B0 )
#define MB_READREG_CNT_MIN              ( 0x0001 )
#define MB_READREG_CNT_MAX              ( 0x007D )
#define MB_WRITEREG_CNT_MIN             ( 0x0001 )
#define MB_WRITEREG_CNT_MAX             ( 0x007B )
#define MB_READWRITE_READREG_CNT_MIN    ( 0x0001 )
#define MB_READWRITE_READREG_CNT_MAX    ( 0x007D )
#define MB_READWRITE_WRITEREG_CNT_MIN   ( 0x0001 )
#define MB_READWRITE_WRITEREG_CNT_MAX   ( 0x0079 )

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    MB_EX_NONE = 0x00,
    MB_EX_ILLEGAL_FUNCTION = 0x01,
    MB_EX_ILLEGAL_DATA_ADDRESS = 0x02,
    MB_EX_ILLEGAL_DATA_VALUE = 0x03,
    MB_EX_SLAVE_DEVICE_FAILURE = 0x04,
    MB_EX_ACKNOWLEDGE = 0x05,
    MB_EX_SLAVE_BUSY = 0x06,
    MB_EX_MEMORY_PARITY_ERROR = 0x08,
    MB_EX_GATEWAY_PATH_FAILED = 0x0A,
    MB_EX_GATEWAY_TGT_FAILED = 0x0B
} eMBException_t;

#ifdef __cplusplus
}
#endif

#endif

