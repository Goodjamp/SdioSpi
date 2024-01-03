#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "SdSpi.h"
#include "SdSpiInternal.h"

static const struct {
    SdSpiCmd cmd;
    SdRespType respType;
} cmdMetaInfo[] = {
    { SD_CMD0, SD_RESPONSE_TYPE_R1},
    { SD_CMD1, SD_RESPONSE_TYPE_R1},
    { SD_CMD41, SD_RESPONSE_TYPE_R1},
    { SD_CMD8, SD_RESPONSE_TYPE_R7},
    { SD_CMD9, SD_RESPONSE_TYPE_R1},
    { SD_CMD10, SD_RESPONSE_TYPE_R1},
    { SD_CMD12, SD_RESPONSE_TYPE_R1B},
    { SD_CMD16, SD_RESPONSE_TYPE_R1},
    { SD_CMD17, SD_RESPONSE_TYPE_R1},
    { SD_CMD18, SD_RESPONSE_TYPE_R1},
    { SD_CMD23, SD_RESPONSE_TYPE_R1},
    { SD_CMD24, SD_RESPONSE_TYPE_R1},
    { SD_CMD25, SD_RESPONSE_TYPE_R1},
    { SD_CMD55, SD_RESPONSE_TYPE_R1},
    { SD_CMD58, SD_RESPONSE_TYPE_R3},
};

/*
 * https://www.ghsi.de/pages/subpages/Online%20CRC%20Calculation/index.php?Polynom=10001001&Message=170102fffe
 * https://rndtool.info/CRC-step-by-step-calculator/
 */

#define SERIALIASE_ARG(BUFF, ARG)     \
    *(BUFF + 0) = (ARG >> 24) & 0Xff; \
    *(BUFF + 1) = (ARG >> 16) & 0Xff; \
    *(BUFF + 2) = (ARG >> 8) & 0Xff;  \
    *(BUFF + 3) = (ARG >> 0) & 0Xff;

#define DESERIALIASE_ARG(BUFF, ARG) \
    ARG |= (*(BUFF + 0) << 24);     \
    ARG |= (*(BUFF + 1) << 16);     \
    ARG |= (*(BUFF + 2) << 8);      \
    ARG |= (*(BUFF + 3) << 0);      \

typedef enum {
    WRITE_TYPE_SINGLE,
    WRITE_TYPE_MULTIPLE_WITH_PRE_ERACING,
    WRITE_TYPE_MULTIPLE_WITHOUT_PRE_ERACING,
} WriteType;

static void sdSpiDelay(SdSpiH *handler, uint32_t delay)
{
    uint32_t startTime = handler->cb.sdSpiGetTimeMs();

    delay++;
    while (delay > (handler->cb.sdSpiGetTimeMs() - startTime))
    {}
}

static uint8_t crc7(uint8_t message[], uint32_t messageSize)
{
    static const uint32_t pol = 0b10001001; //pol = x^7+x^3+x^0
    uint32_t crc = 0;
    uint32_t k;

    while (messageSize-- > 0) {
        k = 8;
        while (k-- > 0) {
            if (crc & (1 << 7)) {
                crc ^= pol;
            }
            crc = (crc << 1) | ((*message >> k) & 1);
        }
        message++;
    }
    k = 8;
    while (k-- > 0) {
        if (crc & (1 << 7)) {
            crc ^= pol;
        }
        crc <<= 1;
    }
    return (crc >> 1) & 0b1111111;
}

static SdSpiIntStatus sdSpiSerializeReq(uint8_t *buffer, SdSpiCmdReq request)
{
    unsigned int argPos= FIELD_OFFSET(SdReqLayout, argument);
    uint32_t arg = 0;

    memset(buffer, 0, sizeof(SdReqLayout));

    switch (request.cmd)
    {
    case SD_CMD1:
    case SD_CMD9:
    case SD_CMD10:
    case SD_CMD12:
    case SD_CMD55:
    case SD_CMD58:
    case SD_CMD0:
        /*
         * The command SD_CMD0 need correct CRC !!
         */
    break;

    case SD_CMD41:
        if (request.cmd41.hcs == SD_HCS_SDHC_SDXC) {
            arg |= 1 << SD_CMD41_HCS_POS;
        }
        break;

    case SD_CMD8:

        /*
         * The command SD_CMD8 need correct CRC !!
         */
        arg |= SD_CMD8_PATTERN_VALUE << SD_CMD8_PATTERN_POS;
        if (request.cmd8.vhs == true) {
            arg |= SD_CMD8_VHS;
        }
        break;

    case SD_CMD16:
        arg = request.cmd16.blockLength;
        break;

    case SD_CMD17:
        arg = request.cmd17.address;
        break;

    case SD_CMD18:
        arg = request.cmd18.address;
        break;

    case SD_CMD23:
        arg = request.cmd23.numberOfBlocks;
        break;

    case SD_CMD24:
        arg = request.cmd24.address;
        break;

    case SD_CMD25:
        arg = request.cmd25.address;
        break;

    default:
        return SD_SPI_UNSUPORTED_COMMAND_ERR_INT_STATUS;
    }

    SERIALIASE_ARG(&buffer[argPos], arg);
    /*
     * The 2 MSB must be 0b01, for example in case of the CMD41, the byte with command
     * index will be:
     *     0b01     Command index
     * 0b   01          101001
     */
    buffer[FIELD_OFFSET(SdReqLayout, cmd)] = (request.cmd ) | (0x1 << 6);

    /*
     * Th 0 bit at the CRC byte must be 1
     */
    buffer[FIELD_OFFSET(SdReqLayout, crc)] = (crc7(buffer, 5) << 1) | 0x1;

    return SD_SPI_OK_INT_STATUS;
}

static SdSpiResult sdSpiDeserializeResp(uint8_t *buffer, SdSpiCmdResp *response, SdRespType respType)

{
    size_t respPos = FIELD_OFFSET(SdRespLayout, resp);
    uint32_t arg = 0;

    DESERIALIASE_ARG(&buffer[respPos], arg);

    memset(response, 0x00, sizeof(SdSpiCmdResp));

    response->r1 = buffer[FIELD_OFFSET(SdRespLayout, r1)];
    response->r1 &= SD_R1_ADDRESS_ERROR | SD_R1_COMMAND_CRC_ERROR |
                   SD_R1_ERASE_RESET | SD_R1_ERASE_SEQUENSE_ERROR |
                   SD_R1_ILIGAL_COMMAND | SD_R1_IDLE_STATE | SD_R1_PARAMETER_ERROR;

    switch (respType) {
    case SD_RESPONSE_TYPE_R3: //this respons type transmite the OCR content

        /*
         * Decode SD card capacity type
         */
        response->R3.cardCapacityStatys =
            BIT_MASK(arg, SD_OCR_CCS_POS, SD_OCR_CCS_MASK) == 0
            ? SD_OCR_CARD_CAPACITY_STATUS_SDSC
            : SD_OCR_CARD_CAPACITY_STATUS_SDHC_SDXC;

        /*
         * Decode switching to voltage
         */
        response->R3.switchingTo18V =
            BIT_MASK(arg, SD_OCR_S18A_POS, SD_OCR_S18A_MASK) == 1;
        response->R3.cardPowerUp =
            BIT_MASK(arg, SD_OCR_POWER_UP_POS, SD_OCR_POWER_UP_MASK) == 1;

        /*
         * Decode voltage
         */
        if (BIT_MASK(arg, SD_OCR_v_27_28_POS, SD_OCR_v_27_28_MASK) == 1) {
            response->R3.vddVoltage = SD_OCR_VDD_VOLTAGE_27_28;
        } else if (BIT_MASK(arg, SD_OCR_v_28_29_POS, SD_OCR_v_28_29_MASK) == 1) {
            response->R3.vddVoltage = SD_OCR_VDD_VOLTAGE_28_29;
        } else if (BIT_MASK(arg, SD_OCR_v_29_30_POS, SD_OCR_v_29_30_MASK) == 1) {
            response->R3.vddVoltage = SD_OCR_VDD_VOLTAGE_29_30;
        } else if (BIT_MASK(arg, SD_OCR_v_30_31_POS, SD_OCR_v_30_31_MASK) == 1) {
            response->R3.vddVoltage = SD_OCR_VDD_VOLTAGE_30_31;
        } else if (BIT_MASK(arg, SD_OCR_v_31_32_POS, SD_OCR_v_31_32_MASK) == 1) {
            response->R3.vddVoltage = SD_OCR_VDD_VOLTAGE_31_32;
        } else if (BIT_MASK(arg, SD_OCR_v_32_33_POS, SD_OCR_v_32_33_MASK) == 1) {
            response->R3.vddVoltage = SD_OCR_VDD_VOLTAGE_32_33;
        } else if (BIT_MASK(arg, SD_OCR_v_33_34_POS, SD_OCR_v_33_34_MASK) == 1) {
            response->R3.vddVoltage = SD_OCR_VDD_VOLTAGE_33_34;
        } else if (BIT_MASK(arg, SD_OCR_v_34_35_POS, SD_OCR_v_34_35_MASK) == 1) {
            response->R3.vddVoltage = SD_OCR_VDD_VOLTAGE_34_35;
        } else if (BIT_MASK(arg, SD_OCR_v_35_36_POS, SD_OCR_v_35_36_MASK) == 1) {
            response->R3.vddVoltage = SD_OCR_VDD_VOLTAGE_35_36;
        }
        break;

    case SD_RESPONSE_TYPE_R7:
        if (BIT_MASK(arg, SD_R7_TEMPLATE_POS, SD_R7_TEMPLATE_MASK)
            != SD_CMD41_PATTERN_VALUE) {
            return SD_SPI_RESPONSE_R7_PATTERN_ERR_INT_STATUS;
        }
        response->R7.voltageAccepted =
            (arg & SD_R7_VHS) != 0;
        break;

    default:
        break;
    }

    return SD_SPI_OK_INT_STATUS;
}

static size_t sdSpiGetRespSize(SdRespType respType)
{
    if (respType == SD_RESPONSE_TYPE_R1 || respType == SD_RESPONSE_TYPE_R1B) {
        return SD_R1_RESP_SIZE;
    } else if (respType == SD_RESPONSE_TYPE_R3) {
        return SD_R3_RESP_SIZE;
    } else if (respType == SD_RESPONSE_TYPE_R7) {
        return SD_R7_RESP_SIZE;
    }

    return 0;
}

static SdRespType sdSpiGetRespType(SdSpiCmd cmd)
{
    for (uint32_t k = 0; k < sizeof(cmdMetaInfo) / sizeof(cmdMetaInfo[0]); k++ ) {
        if (cmd == cmdMetaInfo[k].cmd) {
            return cmdMetaInfo[k].respType;
        }
    }

    return SD_RESPONSE_TYPE_CNT;
}

static SdSpiResult sdSpiWaiteBusy(SdSpiH *handler)
{
    uint8_t buff = 0x00;
    uint32_t startTime = handler->cb.sdSpiGetTimeMs();
    SdSpiInternalTrace *intTrace = (SdSpiInternalTrace *)handler->serviceBuff;

    while (handler->cb.sdSpiGetTimeMs() - startTime < SD_BUSY_TIMEOUTE) {
        if (handler->cb.sdSpiReceive(&buff, 1) == false) {
            return SD_SPI_RESULT_RECEIVE_CB_RETURN_ERROR;
        }
        if (buff != 0x00) {
            break;
        }
    }

    return buff == 0x00
           ? (intTrace->intStatus = SD_SPI_BUSY_TIMEOUTE_ERR_INT_STATUS, SD_SPI_RESULT_INTERNAL_ERROR)
           : SD_SPI_RESULT_OK;
}

/*
 * Return:
 * - SD_RESULT_OK
 * - SD_RESULT_SPI_SET_CS_CB_RETURN_ERROR
 * - SD_RESULT_SPI_SEND_CB_RETURN_ERROR
 * - SD_RESULT_SPI_RECEIVE_CB_RETURN_ERROR
 * - SD_SPI_RESULT_NO_RESPONSE_ERROR
 */
static SdSpiResult sdSpiCmdTransaction(SdSpiH *handler, SdSpiCmdReq request,
                                       SdSpiCmdResp *response, bool keepCs)
{
    SdSpiResult result = SD_SPI_RESULT_OK;
    uint8_t reqBuff[sizeof(SdReqLayout)];
    SdSpiInternalTrace *intTrace = (SdSpiInternalTrace *)handler->serviceBuff;

    /*
     * Serialize request
     */
    intTrace->intStatus = sdSpiSerializeReq(reqBuff, request);
    if (intTrace->intStatus != SD_SPI_OK_INT_STATUS) {
        return SD_SPI_RESULT_INTERNAL_ERROR;
    }

    /*
     * Send request
     */
    if (handler->cb.sdSpiSetCsState(false) == false) {
        return SD_SPI_RESULT_SET_CS_CB_RETURN_ERROR;
    }
    if (handler->cb.sdSpiSend(reqBuff, sizeof(SdReqLayout)) == false) {
        return SD_SPI_RESULT_SEND_CB_RETURN_ERROR;
    }

    /*
     * Receive response
     */
    SdRespType respType = sdSpiGetRespType(request.cmd);
    size_t respSize = sdSpiGetRespSize(respType);
    //uint8_t respBuff[respSize];
    uint8_t respBuff[5];

    memset(respBuff, 0, respSize);

    /*
     * The Ncr time (timeoute response) equal to:
     * - 0 - 8 bytes for the R1, R2, R3, R7 response for the SD card
     * - 1 - 8 bytes for the R1, R2, R3, R7 response for the MMC card
     * Take the maximum bytes number (+2) for the timeote
     */
    for (uint32_t k = 0; k < (SD_WAITE_RESPONSE_IN_BYTES + 2); k++)
    {
        /*
         * In case of the R1b response type we need to continuous
         * polling SD card up to receive non 0xFF value.
         */
        if (handler->cb.sdSpiReceive(&respBuff[FIELD_OFFSET(SdRespLayout, r1)],
                                     FIELD_SIZE(SdRespLayout, r1)) == false) {
            return SD_SPI_RESULT_RECEIVE_CB_RETURN_ERROR;
        }

        /*
         * If the 8-th bit is clear - the R1 response received
         */
        if (respBuff[FIELD_OFFSET(SdRespLayout, r1)] != 0xFF) {
            break;
        }
    }

    /*
     * The card don't reply
     */
    if (respBuff[FIELD_OFFSET(SdRespLayout, r1)] == 0xFF) {
        return SD_SPI_RESULT_NO_RESPONSE_ERROR;
    }

    /*
     * Receive rest part of the reaponse
     */
    if (handler->cb.sdSpiReceive(&respBuff[FIELD_OFFSET(SdRespLayout, resp)],
                                 respSize - FIELD_SIZE(SdRespLayout, r1))
        == false) {
        return SD_SPI_RESULT_RECEIVE_CB_RETURN_ERROR;
    }

    if (handler->cb.sdSpiSetCsState(keepCs) == false) {
        return SD_SPI_RESULT_SET_CS_CB_RETURN_ERROR;
    }

    /*
     * Deserialize response
     */
    intTrace->intStatus = sdSpiDeserializeResp(respBuff, response, respType);
    if (intTrace->intStatus != SD_SPI_OK_INT_STATUS) {
        return SD_SPI_RESULT_INTERNAL_ERROR;
    }

    /*
     * In case of R1b response type, we still need to waite to non zero value from the line
     * before complete using the card
     */
    if (respType == SD_RESPONSE_TYPE_R1B) {
        result = sdSpiWaiteBusy(handler);
    }

    return result;
}

static SdSpiResult sdSpiCardRun(SdSpiH *handler, SdCardVersion sdVersion)
{
    SdSpiCmdReq request;
    SdSpiCmdResp response;
    uint32_t startTime = handler->cb.sdSpiGetTimeMs();
    SdSpiResult result = SD_SPI_RESULT_OK;
    SdSpiInternalTrace *intTrace = (SdSpiInternalTrace *)handler->serviceBuff;

    while (handler->cb.sdSpiGetTimeMs() - startTime < SD_EXIT_IDLE_TIMEOUTE) {
        /*
         * CMD55 is a pre command before send comamnd CMD41
         */
        request.cmd = SD_CMD55;
        result = sdSpiCmdTransaction(handler, request, &response, true);
        if (result != SD_SPI_RESULT_OK) {
            break;
        }
        if (response.r1 != SD_R1_IDLE_STATE) {
            intTrace->intStatus = SD_SPI_CMD55_REPLY_ERR_INT_STATUS;
            result = SD_SPI_RESULT_INTERNAL_ERROR;
            break;
        }

        /*
         * The command depends on the SD card version, see ref. SdVersion
         */
        if (sdVersion == SD_CARD_VERSION_SD_VER_2_PLUS) {
            request.cmd = SD_CMD41;
            request.cmd41.hcs = SD_HCS_SDHC_SDXC;
        } else if (sdVersion == SD_CARD_VERSION_SD_VER_1) {
            request.cmd = SD_CMD41;
            request.cmd41.hcs = SD_HCS_SDSC;
        } else {
            request.cmd = SD_CMD1;
        }
        result = sdSpiCmdTransaction(handler, request, &response, true);
        if (result != SD_SPI_RESULT_OK) {
            break;
        }
        if (response.r1 == 0) {
            break;
        }
        if (response.r1 != SD_R1_IDLE_STATE) {
            intTrace->intStatus = SD_SPI_CMD55_REPLY_ERR_INT_STATUS;
            result = SD_SPI_CMD41_REPLY_ERR_INT_STATUS;
            break;
        }
    }

    return result;
}

SdSpiResult sdSpiInit(SdSpiH *handler, const SdSpiCb *cb)
{
#define SD_SPI_TRANSACTION_BUFF_SIZE           10
    SdSpiResult result;
    SdSpiCmdReq request;
    SdSpiCmdResp response;
    uint8_t transactionBuff[SD_SPI_TRANSACTION_BUFF_SIZE];
    SdSpiInternalTrace *intTrace = (SdSpiInternalTrace *)handler->serviceBuff;

    if (handler == NULL) {
        return SD_SPI_RESULT_HANDLER_NULL_ERROR;
    }
    memset(handler, 0, sizeof(handler));
    if (cb == NULL) {
        return SD_SPI_RESULT_CB_NULL_ERROR;
    }
    if (cb->sdSpiSend == NULL) {
        return SD_SPI_RESULT_SEND_CB_NULL_ERROR;
    }
    if (cb->sdSpiReceive == NULL) {
        return SD_SPI_RESULT_RECEIVE_CB_NULL_ERROR;
    }
    if (cb->sdSpiSetCsState == NULL) {
        return SD_SPI_RESULT_SET_CS_CB_NULL_ERROR;
    }
    if (cb->sdSpiSetSckFrq == NULL) {
        return SD_SPI_RESULT_SET_FRQ_CB_NULL_ERROR;
    }
    if (cb->sdSpiGetTimeMs == NULL) {
        return SD_SPI_RESULT_GET_TIME_CB_NULL_ERROR;
    }
    if (cb->sdSpiMalloc == NULL) {
        return SD_SPI_RESULT_GET_TIME_CB_NULL_ERROR;
    }
    handler->cb = *cb;
    handler->lba = 512;

    uint32_t serviceBuffSize = 0;
#ifdef ENABLE_ERROR_TRACE
    serviceBuffSize += sizeof(SdSpiInternalTrace);
#endif
    if (handler->cb.sdSpiMalloc(serviceBuffSize) == NULL) {
        return SD_SPI_RESULT_MALLOC_CB_RERTURN_NULL_ERROR;
    }
    /*
     * Accordnig to the SD Documentation
     * section: 7.2.1 Mode Selection and Initilisation
     * The initilisation procedure in SPI mode:
     *
     * 1 - Set CS to Hight
     * 2 - Send >= 74 SCK pulces
     * 3 - Set CS to Low
     * 4 - Send command CMD0 - GO_IDLE_STATE
     * 5 - Receive the reply (R1, one byte), the bit IN IDLE STATE must be set
     * 6 - Set CS to Hight
     * 7 - Set CS to Low
     * 8 - Send command CMD8 - SEND_IF_COND
     */
    memset(transactionBuff, 0xFF, sizeof(transactionBuff));
    if (handler->cb.sdSpiSetCsState(true) == false) {
        return SD_SPI_RESULT_SET_CS_CB_RETURN_ERROR;
    }
    if (handler->cb.sdSpiSetSckFrq(SD_SPI_INITIAL_FRQ) == false) {
        return SD_SPI_RESULT_SET_FRQ_CB_RETURN_ERROR;
    }

    /*
     * Waite > 1ms
     */
    sdSpiDelay(handler, SD_SPI_WAITE_STABILE_POWER);

    /*
     * Send > 74 SCK pulces
     */
    if (cb->sdSpiSend(transactionBuff, 10) == false) {
        return SD_SPI_RESULT_SEND_CB_RETURN_ERROR;
    }

    /*
     * Set idle state
     */
    request.cmd = SD_CMD0;
    result = sdSpiCmdTransaction(handler, request, &response, true);
    if (result != SD_SPI_RESULT_OK) {
        return result;
    }
    if (response.r1 != SD_R1_IDLE_STATE) {
        intTrace->intStatus = SD_SPI_CMD55_REPLY_ERR_INT_STATUS;
        return SD_SPI_SET_IDLE_ERR_INT_STATUS;
    }

    /*
     * Try begin initialisation of the SDHC and SDXC card by issue the CMD8
     */
    request.cmd = SD_CMD8;
    request.cmd8.vhs = true;
    result = sdSpiCmdTransaction(handler, request, &response, true);

    if (result == SD_SPI_RESULT_OK
        && response.r1 == SD_R1_IDLE_STATE
        && response.R7.voltageAccepted == true) {

        /*
         * The card is SD Ver.2+ (SDHC or SDXC)
         */
        if (sdSpiCardRun(handler, SD_CARD_VERSION_SD_VER_2_PLUS) == SD_SPI_RESULT_OK) {

            /*
             * Test block length type
             */
            request.cmd = SD_CMD58;
            result = sdSpiCmdTransaction(handler, request, &response, true);
            if (result == SD_SPI_RESULT_OK && response.r1 == 0) {

                /*
                 * If length type equal to Byte, we need to set block length equal to 512
                 */
                if (response.R3.cardCapacityStatys == SD_OCR_CARD_CAPACITY_STATUS_SDSC) {

                    /*
                    * Set block length equal to SDIO_SPI_FAT_LBA (512 byte)
                    */
                    request.cmd = SD_CMD16;
                    request.cmd16.blockLength = SDIO_SPI_FAT_LBA;
                    result = sdSpiCmdTransaction(handler, request, &response, true);
                    if (result == SD_SPI_RESULT_OK && response.r1 != 0)
                    {
                        /*
                         * Faile the cmd16 transaction
                         */
                        intTrace->intStatus = SD_SPI_SET_BLOCK_SIZE_ERR_INT_STATUS;
                        result = SD_SPI_RESULT_INTERNAL_ERROR;
                    }
                } else {
                    handler->lba = 1;
                }
            } else {
                /*
                 * Faile the cmd58 transaction
                 */
                intTrace->intStatus = SD_SPI_CHECK_BLOCK_SIZE_ERR_INT_STATUS;
                result = SD_SPI_RESULT_INTERNAL_ERROR;
            }
        } else {
            intTrace->intStatus = SD_SPI_INIT_VER_2_PLUS_ERR_INT_STATUS;
            result = SD_SPI_RESULT_INTERNAL_ERROR;
        }

    } else if ((result == SD_SPI_RESULT_NO_RESPONSE_ERROR)
               || (result == SD_SPI_RESULT_OK
                   && response.r1 == (SD_R1_IDLE_STATE | SD_R1_ILIGAL_COMMAND))) {
        /*
         * SD Ver 1 OR MMC Ver. 3
         */

        /*
         * First try init SD Ver 1
         */
        result = sdSpiCardRun(handler, SD_CARD_VERSION_SD_VER_1);
        if (result == SD_SPI_RESULT_OK) {

            /*
             * Set block length equal to 512
             */
            request.cmd = SD_CMD16;
            request.cmd16.blockLength = 512;
            result = sdSpiCmdTransaction(handler, request, &response, true);
            if (result == SD_SPI_RESULT_OK && response.r1 != 0)
            {
                /*
                 * Faile the cmd16 transaction
                 */
                intTrace->intStatus = SD_SPI_SET_BLOCK_SIZE_ERR_INT_STATUS;
                result = SD_SPI_RESULT_INTERNAL_ERROR;
            }
        } else {
            /*
             * In case of any type of a error (error reply, no response, timeoute), try init MMC Ver. 3
             */
            result = sdSpiCardRun(handler, SD_CARD_VERSION_MMC_VER_2);
            if (result == SD_SPI_RESULT_OK) {
                /*
                 * Set block length equal to 512
                 */
                request.cmd = SD_CMD16;
                request.cmd16.blockLength = 512;
                result = sdSpiCmdTransaction(handler, request, &response, true);
                if (!(result == SD_SPI_RESULT_OK && response.r1 == 0)) {
                    /*
                     * Faile the cmd16 transaction
                     */
                    intTrace->intStatus = SD_SPI_SET_BLOCK_SIZE_ERR_INT_STATUS;
                    result = SD_SPI_RESULT_INTERNAL_ERROR;
                }
            }
        }
    } else {
        result = SD_SPI_RESULT_UNKNOWN_ERROR;
    }

    return result;
}

static SdSpiIntStatus sdSpiReadBlock(SdSpiH *handler, uint8_t *data)
{
    SdSpiIntStatus result = SD_SPI_OK_INT_STATUS;
    uint32_t k = 0;
    uint8_t dataToken;
    uint8_t crc[SD_DATA_PACKET_CRC_SIZE];

    /*
     * Waite while the SD card start transmit data.
     * Receive Data Token
     */
    while (k < SD_WAITE_DATA_TOKEN_BYTES) {
        handler->cb.sdSpiReceive(&dataToken, sizeof(dataToken));
        if (dataToken == SD_TOKEN_DATA_17_18_24) {
            /*
             * Successfully found the Data token
             */
            break;
        } else if ((dataToken >> 5 & 7) == 0) {
            /*
             * Receive the Error token
             */
            result = SD_SPI_RESULT_RECEIVE_ERROR;
            break;
        }
    }

    if (result != SD_SPI_OK_INT_STATUS) {
        return result;
    }

    /*
     * Receive data;
     */
    handler->cb.sdSpiReceive(data, SDIO_SPI_FAT_LBA);

    /*
     * Receive CRC. We don't test CRC, but need receive it
     */
    handler->cb.sdSpiReceive(crc, sizeof(crc));

    return result;
}

SdSpiResult sdSpiRead(SdSpiH *handler, uint32_t address, uint8_t *data, size_t dataLength)
{
    SdSpiResult result = SD_SPI_OK_INT_STATUS;
    SdSpiCmdReq request;
    SdSpiCmdResp response;
    bool multipleBlock = dataLength > 1;

    if (handler == NULL) {
        return SD_SPI_RESULT_HANDLER_NULL_ERROR;
    }

    if (data == NULL) {
        return SD_SPI_RESULT_DATA_NULL_ERROR;
    }

    if (dataLength == 0) {
        return SD_SPI_RESULT_DATA_LENGTH_ZERO_ERROR;
    }

    /*
     * Send the address from which start read data
     */
    request.cmd = multipleBlock ? SD_CMD18 : SD_CMD17;
    if (multipleBlock) {
        request.cmd18.address = address * handler->lba;
    } else {
        request.cmd17.address = address * handler->lba;
    }
    result = sdSpiCmdTransaction(handler, request, &response, false);
    if (result != SD_SPI_RESULT_OK) {
        return result;
    } else if (response.r1 != 0) {
        return SD_SPI_RESULT_RESPONSE_ERROR;
    }

    /*
     * Read data from the card. The LBA is equal to the SDIO_SPI_FAT_LBA (512 bytes)
     */
    for (uint32_t k = 0; k < dataLength ; k++, data += SDIO_SPI_FAT_LBA) {
        result = sdSpiReadBlock(handler, data);
        if (result != SD_SPI_RESULT_OK) {
            break;
        }
    }

    /*
     * If read more than one LBA, send STOP command
     * to stop data transacrion from the card
     */
    if (dataLength > 1) {
        request.cmd = SD_CMD12;
        result = sdSpiCmdTransaction(handler, request, &response, false);
    }
    handler->cb.sdSpiSetCsState(true);

    return result;
}

static SdSpiResult sdSpiWriteBlock(SdSpiH *handler, uint8_t *data, WriteType writeType)
{
    uint32_t k = 0;
    SdSpiResult result = SD_SPI_RESULT_OK;
    uint8_t dataResponse = 0;
    uint8_t dataToken = (writeType == WRITE_TYPE_SINGLE)
                        ? SD_TOKEN_DATA_17_18_24
                        : SD_TOKEN_DATA_25;
    uint8_t crc[SD_DATA_PACKET_CRC_SIZE] = {0, 0};

    /*
     * Sending data token
     */
    if (handler->cb.sdSpiSend(&dataToken, sizeof(dataToken))
        == false) {
        return SD_SPI_RESULT_SEND_CB_RETURN_ERROR;
    }

    /*
     * Sending data
     */
    if (handler->cb.sdSpiSend(data, SDIO_SPI_FAT_LBA)
        == false) {
        return SD_SPI_RESULT_SEND_CB_RETURN_ERROR;
    }

    /*
     * Sending CRC
     */
    if (handler->cb.sdSpiSend(crc, sizeof(crc))
        == false) {
        return SD_SPI_RESULT_SEND_CB_RETURN_ERROR;
    }

    /*
    * Waite and receive a response, if need
    */
    if (writeType == WRITE_TYPE_MULTIPLE_WITHOUT_PRE_ERACING ||
        writeType == WRITE_TYPE_SINGLE) {

        for (; k < SD_WAITE_DATA_TOKEN_BYTES; k++) {
            handler->cb.sdSpiReceive(&dataResponse, sizeof(dataResponse));
            dataResponse &= SD_WRITE_DATA_RESPONSE_MASK;
            if (dataResponse == SD_WRITE_DATA_RESPONSE_ACCEPTED
                || dataResponse == SD_WRITE_DATA_RESPONSE_CRC_ERROR
                || dataResponse == SD_WRITE_DATA_RESPONSE_WRIRTE_ERROR) {
                break;
            }
        }
        if (k == SD_WAITE_DATA_TOKEN_BYTES) {
            result = SD_SPI_RESULT_NO_RESPONSE_ERROR;
        } else {
            if (dataResponse != SD_WRITE_DATA_RESPONSE_ACCEPTED) {
                result = SD_SPI_RESULT_WRITE_ERROR;
            } else {
                /*
                * Waite to complete busy state
                */
                result = sdSpiWaiteBusy(handler);
            }
        }
    }

    return result;
}

SdSpiResult sdSpiWrite(SdSpiH *handler, uint32_t address, uint8_t *data, size_t dataLength)
{
    SdSpiResult result = SD_SPI_OK_INT_STATUS;
    SdSpiCmdReq request;
    SdSpiCmdResp response;
    WriteType writeType = (dataLength == 1)
                          ? WRITE_TYPE_SINGLE
                          : WRITE_TYPE_MULTIPLE_WITH_PRE_ERACING;
    uint8_t token;

    if (handler == NULL) {
        return SD_SPI_RESULT_HANDLER_NULL_ERROR;
    }

    if (data == NULL) {
        return SD_SPI_RESULT_DATA_NULL_ERROR;
    }

    if (dataLength == 0) {
        return SD_SPI_RESULT_DATA_LENGTH_ZERO_ERROR;
    }

    /*
     * Send the address from which start write data
     */
    request.cmd = (writeType == WRITE_TYPE_SINGLE) ? SD_CMD25 : SD_CMD24;
    if (writeType == WRITE_TYPE_SINGLE) {
        request.cmd24.address = address * handler->lba;
    } else {
        request.cmd25.address = address * handler->lba;
    }
    result = sdSpiCmdTransaction(handler, request, &response, false);
    if (result != SD_SPI_RESULT_OK) {
        return result;
    } else if (response.r1 != 0) {
        return SD_SPI_RESULT_RESPONSE_ERROR;
    }

    /*
     * waite > 1 byte
     */
    sdSpiDelay(handler, 1);

    /*
     * Inform about quantity of the write bloks for the case of multiple block write
     */
    if (writeType != WRITE_TYPE_SINGLE) {
        /*
         * CMD55 is a pre command before send comamnd CMD23
         */
        request.cmd = SD_CMD55;
        result = sdSpiCmdTransaction(handler, request, &response, true);
        if (result != SD_SPI_RESULT_OK) {
            return result;
        } else if (response.r1 != 0) {
            return SD_SPI_RESULT_RESPONSE_ERROR;
        }

        request.cmd = SD_CMD23;
        request.cmd23.numberOfBlocks = dataLength;
        result = sdSpiCmdTransaction(handler, request, &response, false);
        if (result != SD_SPI_RESULT_OK) {
            return result;
        } else if (response.r1 != 0){
            /*
             * Some card does not support the CMD23. Analysing a response.
             */
            if (response.r1 == SD_R1_ILIGAL_COMMAND) {
                writeType = WRITE_TYPE_MULTIPLE_WITHOUT_PRE_ERACING;
            } else {
                return SD_SPI_RESULT_RESPONSE_ERROR;
            }
        }
    }

    /*
     * The next type of writing support:
     * 1 - multiple block: pre-defined multiple block write
     * 2 - multiple block: without pre eracing command
     * 3 - single block write
     */

    /*
     * Write data to the card
     */
    for (uint32_t k = 0; k < dataLength ; k++, data += SDIO_SPI_FAT_LBA) {
        result = sdSpiWriteBlock(handler, data, result);
        if (result != SD_SPI_RESULT_OK) {
            break;
        }
    }

    if (result == SD_SPI_RESULT_NO_RESPONSE_ERROR
        || result == SD_SPI_RESULT_SEND_CB_RETURN_ERROR) {
        return result;
    }

    /*
     * waite > 1 byte
     */
    sdSpiDelay(handler, 1);

    /*
     * If write more than one LBA, send STOP TRAN token
     */
    token = SD_TOKEN_STOP_TRAN;
    if (handler->cb.sdSpiSend(&token, TOKEN_SIZE) == false) {
        result = SD_SPI_RESULT_SEND_CB_RETURN_ERROR;
    }

    return result;
}