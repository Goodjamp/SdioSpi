#ifndef __SD_SPI_INTERNAL_H__
#define __SD_SPI_INTERNAL_H__

#include <stdint.h>
#include <stdbool.h>

#include "SdSpi.h"

#define ENABLE_ERROR_TRACE

typedef enum {
    SD_SPI_OK_INT_STATUS,
    SD_SPI_UNSUPORTED_COMMAND_ERR_INT_STATUS,
    SD_SPI_SET_IDLE_ERR_INT_STATUS,
    SD_SPI_RESPONSE_R7_PATTERN_ERR_INT_STATUS,
    SD_SPI_CMD55_NO_REPLY_ERR_INT_STATUS,
    SD_SPI_CMD55_REPLY_ERR_INT_STATUS,
    SD_SPI_CMD41_PROCESSING_ERR_INT_STATUS,
    SD_SPI_CMD41_REPLY_ERR_INT_STATUS,
    SD_SPI_SD_VER2_PLUS_INIT_ERR_INT_STATUS,
    SD_SPI_RUN_TIMEOUTE_ERR_INT_STATUS,
    SD_SPI_BUSY_TIMEOUTE_ERR_INT_STATUS,
    SD_SPI_SET_BLOCK_SIZE_ERR_INT_STATUS,
    SD_SPI_CHECK_BLOCK_SIZE_ERR_INT_STATUS,
    SD_SPI_INIT_VER_2_PLUS_ERR_INT_STATUS,
} SdSpiIntStatus;

typedef struct {
    SdSpiIntStatus intStatus;
} SdSpiInternalTrace;

/*
 * Definitions:
 * LBA - logical block length
 * HCS - hos capasity status
 * CHS - voltage host supply
 * OCR - operation condition register
 * s18a - switch to 1.8 accepted
 * CCS - card capacity status
 * VHS - voltage host supply
 */

/*
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | Command| Argument               | Resp|Data |      Abbreviation        | Description                                         |
 * | Index  |                        |     |     |                          |                                                     |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD0   | None(0)                | R1  | No  | GO_IDLE_STATE            | Software reset                                      |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD1   | None(0)                | R1  | No  | SEND_OP_COND             | Initiate initialization process                     |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | ACMD41 | *2                     | R1  | No  | APP_SEND_OP_COND         | For only SDC. Initiate initialization process.      |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD8   | *3                     | R7  | No  | SEND_IF_COND             | For only SDC V2. Check voltage range and            |
 * |        |                        |     |     |                          |Reviving CMD8 enables to expansion of new            |
 * |        |                        |     |     |                          |functionality to some existing commands by redefining|
 * |        |                        |     |     |                          |previously reserved bits. ACMD41 was expanded to     |
 * |        |                        |     |     |                          |support the initialization of the SDHC Card and the  |
 * |        |                        |     |     |                          |expansion is also applied to the SDXC Card.          |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD9   | None(0)                | R1  | Yes | SEND_CSD                 | Read CSD register                                   |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD10  | None(0)                | R1  | Yes | SEND_CID                 | Read CID register                                   |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD12  | None(0)                | R1b | No  | STOP_TRANSMISSION        | Stop to read data                                   |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD16  | Block length[31:0]     | R1  | No  | SET_BLOCKLEN             | Change R/W block size                               |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD17  | Address[31:0]          | R1  | Yes | READ_SINGLE_BLOCK        | Read a block                                        |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD18  | Address[31:0]          | R1  | Yes | READ_MULTIPLE_BLOCK      | Read multiple blocks                                |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD23  | Number of blocks[15:0] | R1  | No  | SET_BLOCK_COUNT          | For only MMC. Define number of blocks to transfer   |
 * |        |                        |     |     |                          | with next multi-block read/write command            |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | ACMD23 | Number of blocks[22:0] | R1  | No  | SET_WR_BLOCK_ERASE_COUNT | For only SDC. Define number of blocks to pre-erase  |
 * |        |                        |     |     |                          | with next multi-block write command                 |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD24  | Address[31:0]          | R1  | Yes | WRITE_BLOCK              | Write a block                                       |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD25  | Address[31:0]          | R1  | Yes | WRITE_MULTIPLE_BLOCK     | Write multiple blocks                               |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD55  | None(0)                | R1  | No  | APP_CMD                  | Leading command of ACMD<n> command                  |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD58  | None(0)                | R3  | No  | READ_OCR                 | Read OCR                                            |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+

 */

#define SDIO_SPI_FAT_LBA                       512
#define SD_SPI_WAITE_STABILE_POWER             2

#define SD_SPI_INITIAL_FRQ                     100000
#define SD_SPI_FAST_FRQ                        20000000
#define SD_EXIT_IDLE_TIMEOUTE                  100
#define SD_BUSY_TIMEOUTE                       200
#define SD_WAITE_RESPONSE_IN_BYTES             8
#define SD_WAITE_DATA_TOKEN_BYTES              1000

#define SD_R1_MASK                             0x7F

#define SD_DATA_PACKET_CRC_SIZE                2

/*
 * Data token
 */
#define TOKEN_SIZE                             1
#define SD_TOKEN_DATA_17_18_24                 254
#define SD_TOKEN_DATA_25                       252
#define SD_TOKEN_STOP_TRAN                     253
/*
 * Returned on the head of the data packet in case of any error
 * of the read operation
 */
#define SD_SPI_TOKEN_ERROR                     31

#define SD_WRITE_DATA_RESPONSE_MASK            31
#define SD_WRITE_DATA_RESPONSE_ACCEPTED        5
#define SD_WRITE_DATA_RESPONSE_CRC_ERROR       11
#define SD_WRITE_DATA_RESPONSE_WRIRTE_ERROR    13

/*
 * R1 response Idle state
 */
#define SD_R1_IDLE_STATE_POS                   0
#define SD_R1_IDLE_STATE_MASK                  1
#define SD_R1_IDLE_STATE                       (SD_R1_IDLE_STATE_MASK << SD_R1_IDLE_STATE_POS)

/*
 * R1 response Erase reset
 */
#define SD_R1_ERASE_RESET_POS                  1
#define SD_R1_ERASE_RESET_MASK                 1
#define SD_R1_ERASE_RESET                      (SD_R1_ERASE_RESET_MASK << SD_R1_ERASE_RESET_POS)

/*
 * R1 response Iligal command
 */
#define SD_R1_ILIGAL_COMMAND_POS               2
#define SD_R1_ILIGAL_COMMAND_MASK              1
#define SD_R1_ILIGAL_COMMAND                   (SD_R1_ILIGAL_COMMAND_MASK << SD_R1_ILIGAL_COMMAND_POS)

/*
 * R1 response Command CRC error
 */
#define SD_R1_COMMAND_CRC_ERROR_POS            3
#define SD_R1_COMMAND_CRC_ERROR_MASK           1
#define SD_R1_COMMAND_CRC_ERROR                (SD_R1_COMMAND_CRC_ERROR_MASK \
                                                << SD_R1_COMMAND_CRC_ERROR_POS)

/*
 * R1 response Erace sequense error
 */
#define SD_R1_ERASE_SEQUENSE_ERROR_POS         4
#define SD_R1_ERASE_SEQUENSE_ERROR_MASK        1
#define SD_R1_ERASE_SEQUENSE_ERROR             (SD_R1_ERASE_SEQUENSE_ERROR_MASK \
                                                << SD_R1_ERASE_SEQUENSE_ERROR_POS)

/*
 * R1 response Address error
 */
#define SD_R1_ADDRESS_ERROR_POS                5
#define SD_R1_ADDRESS_ERROR_MASK               1
#define SD_R1_ADDRESS_ERROR                    (SD_R1_ADDRESS_ERROR_MASK << SD_R1_ADDRESS_ERROR_POS)

/*
 * R1 response Address error
 */
#define SD_R1_ADDRESS_ERROR_POS                5
#define SD_R1_ADDRESS_ERROR_MASK               1
#define SD_R1_ADDRESS_ERROR                    (SD_R1_ADDRESS_ERROR_MASK << SD_R1_ADDRESS_ERROR_POS)

/*
 * R1 response Parameter error
 */
#define SD_R1_PARAMETER_ERROR_POS              6
#define SD_R1_PARAMETER_ERROR_MASK             1
#define SD_R1_PARAMETER_ERROR                  (SD_R1_PARAMETER_ERROR_MASK << SD_R1_PARAMETER_ERROR_POS)

/*
 * R7 response VHS
 */
#define SD_R7_VHS_POS                          8
#define SD_R7_VHS_MASK                         1
#define SD_R7_VHS                              (SD_R7_VHS_MASK << SD_R7_VHS_POS)

/*
 * R7 response template
 */
#define SD_R7_TEMPLATE_POS                     0
#define SD_R7_TEMPLATE_MASK                    0xFF

/*
 * CMD8 request VHS
 */
#define SD_CMD8_VHS_POS                        8
#define SD_CMD8_VHS_MASK                       1
#define SD_CMD8_VHS                            (SD_CMD8_VHS_MASK << SD_CMD8_VHS_POS)

/*
 * CMD8 pattern
 */
#define SD_CMD8_PATTERN_POS                    0
#define SD_CMD8_PATTERN_MASK                   0xFF
#define SD_CMD8_PATTERN                        (SD_CMD8_PATTERN_MASK << SD_CMD8_PATTERN_POS)
#define SD_CMD8_PATTERN_VALUE                  0xAA

/*
 * CMD41 request HCS
 */
#define SD_CMD41_HCS_POS                       30
#define SD_CMD41_HCS_MASK                      1
#define SD_CMD41_HCS                           (SD_CMD41_HCS_MASK << SD_CMD41_HCS_POS)

/*
 * CMD41 pattern
 */
#define SD_CMD41_PATTERN_POS                   0
#define SD_CMD41_PATTERN_MASK                  0xFF
#define SD_CMD41_PATTERN                       (SD_CMD41_PATTERN_MASK << SD_CMD41_PATTERN_POS)
#define SD_CMD41_PATTERN_VALUE                 0xAA

/*
 * OCR bit 2.7-2.8 volts
 */
#define SD_OCR_v_27_28_POS                     15
#define SD_OCR_v_27_28_MASK                    1

/*
 * OCR bit 2.8-2.9 volts
 */
#define SD_OCR_v_28_29_POS                     16
#define SD_OCR_v_28_29_MASK                    1

/*
 * OCR bit 2.9-30 volts
 */
#define SD_OCR_v_29_30_POS                     17
#define SD_OCR_v_29_30_MASK                    1

/*
 * OCR bit 3.0-3.1 volts
 */
#define SD_OCR_v_30_31_POS                     18
#define SD_OCR_v_30_31_MASK                    1

/*
 * OCR bit 3.1-3.2 volts
 */
#define SD_OCR_v_31_32_POS                     19
#define SD_OCR_v_31_32_MASK                    1

/*
 * OCR bit 3.2-3.3 volts
 */
#define SD_OCR_v_32_33_POS                     20
#define SD_OCR_v_32_33_MASK                    1

/*
 * OCR bit 3.3-3.4 volts
 */
#define SD_OCR_v_33_34_POS                     21
#define SD_OCR_v_33_34_MASK                    1

/*
 * OCR bit 3.4-35 volts
 */
#define SD_OCR_v_34_35_POS                     22
#define SD_OCR_v_34_35_MASK                    1

/*
 * OCR bit 3.5-3.6 volts
 */
#define SD_OCR_v_35_36_POS                     23
#define SD_OCR_v_35_36_MASK                    1

/*
 * OCR bit switched to 1.8V Accepted
 */
#define SD_OCR_S18A_POS                        24
#define SD_OCR_S18A_MASK                       1

/*
 * OCR bit UHS-II Card status
 */
#define SD_OCR_UHS_CS_POS                      29
#define SD_OCR_UHS_CS_MASK                     1

/*
 * OCR bit Card capacity status
 */
#define SD_OCR_CCS_POS                         30
#define SD_OCR_CCS_MASK                        1

/*
 * OCR bit Card power up (busy)
 */
#define SD_OCR_POWER_UP_POS                    31
#define SD_OCR_POWER_UP_MASK                   1



#define SD_R1_RESP_SIZE                        1
#define SD_R3_RESP_SIZE                        5
#define SD_R7_RESP_SIZE                        5

#define COMMAND_VAL(COMMAND_ID)                ((01 << 6) |  COMMAND_ID)
#define COMMAND_VAL(COMMAND_ID)                ((01 << 6) |  COMMAND_ID)
#define BIT_MASK(VAL, POS, MASK)               (MASK & (VAL >> POS))
#define FIELD_OFFSET(STR, F)                   ((uint32_t)&(*((STR *)0)).F)
#define FIELD_SIZE(STR, F)                     sizeof(((STR *)0)->F)

#pragma pack(push, 1)
typedef struct {
    uint8_t cmd;
    uint32_t argument;
    uint8_t crc;
} SdReqLayout;

typedef struct {
    uint8_t r1;
    uint32_t resp;
} SdRespLayout;
#pragma pack(pop)

typedef enum {
    SD_RESPONSE_TYPE_R1,
    SD_RESPONSE_TYPE_R3,
    SD_RESPONSE_TYPE_R7,
    SD_RESPONSE_TYPE_R1B,
    SD_RESPONSE_TYPE_CNT,
} SdRespType;

typedef enum {
    SD_CMD0 = 0,
    SD_CMD1 = 1,
    SD_CMD41 = 41,
    SD_CMD8 = 8,
    SD_CMD9 = 9,
    SD_CMD10 = 10,
    SD_CMD12 = 12,
    SD_CMD16 = 16,
    SD_CMD17 = 17,
    SD_CMD18 = 18,
    SD_CMD23 = 23,
    SD_CMD24 = 24,
    SD_CMD25 = 25,
    SD_CMD55 = 55,
    SD_CMD58 = 58,
} SdSpiCmd;

typedef enum {
    SD_OCR_VDD_VOLTAGE_27_28,
    SD_OCR_VDD_VOLTAGE_28_29,
    SD_OCR_VDD_VOLTAGE_29_30,
    SD_OCR_VDD_VOLTAGE_30_31,
    SD_OCR_VDD_VOLTAGE_31_32,
    SD_OCR_VDD_VOLTAGE_32_33,
    SD_OCR_VDD_VOLTAGE_33_34,
    SD_OCR_VDD_VOLTAGE_34_35,
    SD_OCR_VDD_VOLTAGE_35_36,
} SdOcrVddVoltage;

typedef enum {
    SD_OCR_CARD_CAPACITY_STATUS_SDSC,         // The LBA set by the command CMD16
    SD_OCR_CARD_CAPACITY_STATUS_SDHC_SDXC,    // The LBA has fixed size 512 byte
} SdOcrCardCapacityStatys;

typedef enum {
    SD_HCS_SDSC,         // The LBA set by the command CMD16
    SD_HCS_SDHC_SDXC,    // The LBA has fixed size 512 byte
} SdHcs;

typedef struct {
    SdSpiCmd cmd;
    union {
        struct {
            SdHcs hcs;
        } cmd41;
        struct {
            bool vhs;
        } cmd8;
        struct {
            uint32_t blockLength;
        } cmd16;
        struct {
            uint32_t address;
        } cmd17;
        struct {
            uint32_t address;
        } cmd18;
        struct {
            uint16_t numberOfBlocks;
        } cmd23;
        struct {
            uint32_t address;
        } cmd24;
        struct {
            uint32_t address;
        } cmd25;
    };
} SdSpiCmdReq;

typedef struct {
    /*
     * The next fieald represent bits of R1 response
     */
    uint8_t r1;

    union
    {
        /*
         * The next struct represent R7 response without R1 bits
         */
        struct {
            bool voltageAccepted;
        } R7;

        /*
         * The next struct represent R3 response without R1 bits
         */
        struct {
            SdOcrVddVoltage vddVoltage;
            SdOcrCardCapacityStatys cardCapacityStatys;
            bool switchingTo18V;
            bool cardPowerUp;
        } R3;
    };
} SdSpiCmdResp;

#pragma pack(push, 1)
typedef struct {
    uint32_t one:1;
    uint32_t crc:7;
    uint32_t reserved2:2;
    uint32_t fileFormat:2;
    uint32_t tempWriteProtection:1;
    uint32_t permWriteProtection:1;
    uint32_t copyFlag:1;
    uint32_t fileFormatGroupe:1;
    uint32_t reserved3:5;
    uint32_t partBlocksForWriteAllowed:1;
    uint32_t maxWriteDataBlockLen:4;
    uint32_t writeSpeedFactor:3;
    uint32_t reserved4:2;
    uint32_t writeProtectGroupEnable:1;

    uint16_t writeProtectGroupSize:7;
    uint16_t eraseSectorSize:7;
    uint16_t eraseSingleBlockEnable:1;
    uint16_t reserved5:1;

    uint32_t deviceSize:22;
    uint32_t reseved6:6;
    uint32_t dsrImplemented:1;
    uint32_t readBlockMisalignment:1;
    uint32_t writeBlockMisalignment:1;
    uint32_t partialBlockForReadAllowed:1;

    uint32_t maxReadDataBlockLen:4;
    uint32_t cardCommandClasses:12;
    uint32_t maxDataTransferRate:8;
    uint32_t dataReadAccesTimeInClk:8;

    uint16_t dataReadAccesTime:8;
    uint16_t reserved6:6;
    uint16_t csdStruct:2;
} SdSpiCsdV2;


typedef struct {
    uint8_t one:1;
    uint8_t crc:7;

    uint16_t ManufacturingDate:12;
    uint16_t reserved1:4;

    uint32_t serialNumber;
    uint8_t revision;

    uint8_t name[5];

    uint16_t applicationId;
    uint8_t manufacturerId;
} SdSpiCid;

#pragma pack(pop)

#endif // __SD_SPI_INTERNAL_H__