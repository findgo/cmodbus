
#include "mbutils.h"
#include <assert.h>

#define BITS_uint8_t (8U)

/*! \brief Function to set bits in a byte buffer.
 *
 * This function allows the efficient use of an array to implement bitfields.
 * The array used for storing the bits must always be a multiple of two
 * bytes. Up to eight bits can be set or cleared in one operation.
 *
 * @param : byteBuf A buffer where the bit values are stored.
 *          Must be a multiple of 2 bytes. No length checking is performed and if
 *          usBitOffset / 8 is greater than the size of the buffer memory contents is overwritten.
 *
 * @param : bitOffset The starting address of the bits to set. The first  bit has the offset 0.
 * @param : nBits Number of bits to modify. The value must always be smaller than 8.
 * @param : values Thew new values for the bits. The value for the first bit
 *   starting at <code> usBitOffset </code> is the LSB of the value
 *   <code> ucValues </code>
 *
 * \! code
 * ucBits[2] = {0, 0};
 *
 * // Set bit 4 to 1 (read: set 1 bit starting at bit offset 4 to value 1)
 * MbSetBits( ucBits, 4, 1, 1 );
 *
 * // Set bit 7 to 1 and bit 8 to 0.
 * MbSetBits( ucBits, 7, 2, 0x01 );
 *
 * // Set bits 8 - 11 to 0x05 and bits 12 - 15 to 0x0A;
 * MbSetBits( ucBits, 8, 8, 0x5A);
 * \! endcode
 */
void MbSetBits(uint8_t *byteBuf, uint16_t bitOffset, uint8_t nBits, uint8_t value) {
    uint16_t word;
    uint16_t mask;
    uint16_t byteOffset;
    uint16_t preBits;
    uint16_t newValue = value;

    assert(nBits <= 8);
    assert((size_t) BITS_uint8_t == sizeof(uint8_t) * 8);

    /* Calculate byte offset for first byte containing the bit values starting at bitOffset. */
    byteOffset = (uint16_t) ((bitOffset) / BITS_uint8_t);
    /* How many bits precede our bits to set. */
    preBits = (uint16_t) (bitOffset - byteOffset * BITS_uint8_t);
    /* Move bit field into position over bits to set */
    newValue <<= preBits;

    /* Prepare a mask for setting the new bits. */
    mask = (uint16_t) ((1 << (uint16_t) nBits) - 1);
    mask <<= preBits;
    // Prepare a value want to change
    newValue &= mask;

    /* copy bits into temporary storage. */
    word = byteBuf[byteOffset];
    if ((preBits + nBits) > 8) {
        word |= (uint16_t) byteBuf[byteOffset + 1] << BITS_uint8_t;
    }

    /* Zero out bit field bits and then or value bits into them. */
    word = (uint16_t) ((word & (~mask)) | newValue);

    /* move bits back into storage */
    byteBuf[byteOffset] = (uint8_t) (word & 0xFF);
    if ((preBits + nBits) > 8) {
        byteBuf[byteOffset + 1] = (uint8_t) (word >> BITS_uint8_t);
    }
}

/*! \brief Function to read bits in a byte buffer.
 *
 * This function is used to extract up bit values from an array.
 * Up to eight bit values can be extracted in one step.
 *
 * @param : ucByteBuf A buffer where the bit values are stored.
 * @param : usBitOffset The starting address of the bits to set. The first bit has the offset 0.
 * @param : ucNBits Number of bits to modify. The value must always be smaller than 8.
 *
 * \! code
 * uint8_t ucBits[2] = {0, 0};
 * uint8_t ucResult;
 *
 * /! Extract the bits 3 - 10.
 * ucResult = MbGetBits( ucBits, 3, 8 );
 * \! endcode
 */
uint8_t MbGetBits(uint8_t *byteBuf, uint16_t bitOffset, uint8_t nBits) {
    uint16_t word;
    uint16_t mask;
    uint16_t byteOffset;
    uint16_t preBits;

    /* Calculate byte offset for first byte containing the bit values starting at bitOffset. */
    byteOffset = (uint16_t) ((bitOffset) / BITS_uint8_t);
    /* How many bits precede our bits to set. */
    preBits = (uint16_t) (bitOffset - byteOffset * BITS_uint8_t);

    /* Prepare a mask for setting the new bits. */
    mask = (uint16_t) ((1 << (uint16_t) nBits) - 1);

    /* copy bits into temporary storage. */
    word = byteBuf[byteOffset];
    if ((preBits + nBits) > 8) {
        word |= byteBuf[byteOffset + 1] << BITS_uint8_t;
    }
    /* throw away unneeded bits. */
    word >>= preBits;
    /* mask away bits above the requested bitfield. */
    word &= mask;

    return (uint8_t) word;
}

#if MB_RTU_ENABLED > 0
static const uint8_t aucCRCHi[] = {
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40
};

static const uint8_t aucCRCLo[] = {
        0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
        0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
        0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
        0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
        0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
        0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
        0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
        0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
        0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
        0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
        0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
        0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
        0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
        0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
        0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
        0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
        0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
        0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
        0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
        0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
        0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
        0x41, 0x81, 0x80, 0x40
};

uint16_t MbCRC16(uint8_t *pFrame, uint16_t len) {

    uint8_t CRCHi = 0xFF;
    uint8_t CRCLo = 0xFF;
    int iIndex;

    while (len--) {
        iIndex = CRCLo ^ *(pFrame++);
        CRCLo = (uint8_t) (CRCHi ^ aucCRCHi[iIndex]);
        CRCHi = aucCRCLo[iIndex];
    }

    return (uint16_t) (CRCHi << 8 | CRCLo);
}

#endif

#if MB_ASCII_ENABLED > 0

uint8_t MbChar2Bin(uint8_t character) {
    if ((character >= '0') && (character <= '9')) {
        return (uint8_t) (character - '0');
    } else if ((character >= 'A') && (character <= 'F')) {
        return (uint8_t) (character - 'A' + 0x0A);
    } else {
        return 0xFF;
    }
}

uint8_t MbBin2Char(uint8_t byte) {
    if (byte <= 0x09) {
        return (uint8_t) ('0' + byte);
    } else if ((byte >= 0x0A) && (byte <= 0x0F)) {
        return (uint8_t) (byte - 0x0A + 'A');
    } else {
        /* Programming error. */
        assert(0);
    }

    return '0';
}


uint8_t MbLRC(uint8_t *pFrame, uint16_t len) {
    uint8_t ucLRC = 0;  /* LRC char initialized */

    while (len--) {
        ucLRC += *pFrame++;   /* Add buffer byte without carry */
    }

    /* Return twos complement */
    return (uint8_t) (-((char) ucLRC));
}

#endif

MbException_t MbError2Exception(MbErrCode_t errorCode) {
    switch (errorCode) {
        case MB_ESUCCESS:
            return MB_EX_NONE;

        case MB_ENOREG:
            return MB_EX_ILLEGAL_DATA_ADDRESS;

        case MB_ETIMEDOUT:
            return MB_EX_SLAVE_BUSY;

        default:
            return MB_EX_SLAVE_DEVICE_FAILURE;
    }
}

const char *MbError2Str(MbException_t exCode) {
    switch (exCode) {
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

/**
 * @brief       计算寄存器所占内存字节数
 * @param       holdingNum -  保持寄存器个数
 * @param       inputNum -  输入寄存器个数
 * @param       coilsNum - 线圈个数
 * @param       discreteNum - 离散输入个数
 * @return      返回寄存器占用的总内存数
 */
uint32_t MbRegBufSizeCal(uint16_t holdingNum, uint16_t inputNum, uint16_t coilsNum, uint16_t discreteNum) {
    uint32_t size;

    size = holdingNum * sizeof(uint16_t) + inputNum * sizeof(uint16_t);
    size += (coilsNum >> 3) + (((coilsNum & 0x07) > 0) ? 1 : 0);
    size += (discreteNum >> 3) + (((discreteNum & 0x07) > 0) ? 1 : 0);

    return size;
}
                               
