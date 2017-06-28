
#ifndef _MB_FRAME_H
#define _MB_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Constants which defines the format of a modbus frame. The example is
 * shown for a Modbus RTU/ASCII frame. Note that the Modbus PDU is not
 * dependent on the underlying transport.
 *
 * <code>
 * <------------------------ MODBUS SERIAL LINE ADU (1) ------------------->
 *              <----------- MODBUS PDU (1') ---------------->
 *  +-----------+---------------+----------------------------+-------------+
 *  | Address   | Function Code | Data                       | CRC/LRC     |
 *  +-----------+---------------+----------------------------+-------------+
 *  |           |               |                                   |
 * (2)        (3/2')           (3')                                (4)
 *
 * (1)  ... MB_ADU_SIZE_MAX = 256
 * (2)  ... MB_SER_ADU_ADDR_OFFSET = 0
 * (3)  ... MB_SER_ADU_PDU_OFFSET  = 1
 * (4)  ... MB_SER_ADU_SIZE_CRC = 2
 *      ... MB_SER_ADU_SIZE_LRC = 1
 *
 * (1') ... MB_PDU_SIZE_MAX     = 253
 * (2') ... MB_PDU_FUNC_OFFSET     = 0
 * (3') ... MB_PDU_DATA_OFFSET     = 1
 * </code>
 */
 
 /*!
 * <------------------------ MODBUS TCP/IP ADU(1) ------------------------->
 *                              <----------- MODBUS PDU (1') -------------->
 *  +-----------+---------------+------------------------------------------+
 *  | TID | PID | Length | UID  | Function Code  | Data                    |
 *  +-----------+---------------+------------------------------------------+
 *  |     |     |        |      |                                           
 * (2)   (3)   (4)      (5)    (6)                                          
 *
 * (2)  ... MB_TCP_ADU_TID_OFFSET          = 0 (Transaction Identifier - 2 Byte) 
 * (3)  ... MB_TCP_ADU_PID_OFFSET          = 2 (Protocol Identifier - 2 Byte)
 * (4)  ... MB_TCP_ADU_LEN_OFFSET          = 4 (Number of bytes - 2 Byte)
 * (5)  ... MB_TCP_ADU_UID_OFFSET          = 6 (Unit Identifier - 1 Byte)
 * (6)  ... MB_TCP_ADU_PDU_OFFSET          = 7 (Modbus PDU )
 *
 * (1)  ... Modbus TCP/IP Application Data Unit
 * (1') ... Modbus Protocol Data Unit
 */

/* RS232 / RS485 ADU -- TCP MODBUS ADU */
#define MB_ADU_SIZE_MAX           256
#define MB_ADU_ASCII_SIZE_MIN     3       /*!< Minimum size of a Modbus ASCII frame. */
#define MB_ADU_RTU_SIZE_MIN       4       /*!< Minimum size of a Modbus RTU frame. */
#define MB_ADU_TCP_SIZE_MIN       8       /*!< Minimum size of a Modbus TCP frame. */

/* MODBUS SERIAL RTU/ASCII Defines*/
#define MB_SER_ADU_SIZE_ADDR   1  /*!< Size of ADDRESS field in ADU. */
#define MB_SER_ADU_SIZE_CRC    2  /*!< Size of CRC field in ADU. */
#define MB_SER_ADU_SIZE_LRC    1  /*!< Size of CRC field in ADU. */
#define MB_SER_ADU_ADDR_OFFSET 0  /*!< Offset of slave address in Ser-ADU. */
#define MB_SER_ADU_PDU_OFFSET  1  /*!< Offset of Modbus-PDU in Ser-ADU. */

/* MODBUS TCP  ADU Defines*/
#define MB_TCP_ADU_SIZE_MBAP           7   /*!< Size of MBAP header field in ADU. */
#define MB_TCP_ADU_TID_OFFSET          0
#define MB_TCP_ADU_PID_OFFSET          2
#define MB_TCP_ADU_LEN_OFFSET          4
#define MB_TCP_ADU_UID_OFFSET          6
#define MB_TCP_ADU_PDU_OFFSET          7

/* ----------------------- Defines ------------------------------------------*/
#define MB_PDU_SIZE_MAX         253 /*!< Maximum size of a PDU. */
#define MB_PDU_SIZE_MIN         1   /*!< Function Code */
#define MB_PDU_SIZE_FUNCODE     1   /*!< Size of Function Code in PDU */
#define MB_PDU_FUNCODE_OFF      0   /*!< Offset of function code in PDU. */
#define MB_PDU_DATA_OFF         1   /*!< Offset for response data in PDU. */

#ifdef __cplusplus
}
#endif
#endif

