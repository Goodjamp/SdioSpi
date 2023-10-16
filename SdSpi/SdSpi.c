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

static SdioSpiIntStatus sdSpiSerializeReq(uint8_t *buffer, SdSpiCmdReq request)
{
    unsigned int argPos= FIELD_OFFSET(ReqLayout, argument);

    memset(buffer, 0, sizeof(ReqLayout));

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
        if (request.cmd41.hcs == HCS_SDHC_SDXC) {
            buffer[argPos] |= 1 << SD_CMD41_HCS_POS;
        }
        break;

    case SD_CMD8:
        /*
         * The command SD_CMD8 need correct CRC !!
         */
        buffer[argPos] |= SD_CMD8_PATTERN_VALUE << SD_CMD8_PATTERN_POS;
        if (request.cmd8.vhs == true) {
            buffer[argPos] |= 1 << SD_CMD8_VHS_POS;
        }
        break;

    case SD_CMD16:
        memcpy(&buffer[argPos], &request.cmd16.blockLength, sizeof(request.cmd16.blockLength));
        break;

    case SD_CMD17:
        memcpy(&buffer[argPos], &request.cmd17.address, sizeof(request.cmd17.address));
        break;

    case SD_CMD18:
        memcpy(&buffer[argPos], &request.cmd18.address, sizeof(request.cmd18.address));
        break;

    case SD_CMD23:
        memcpy(&buffer[argPos], &request.cmd23.numberOfBlocks,
               sizeof(request.cmd23.numberOfBlocks));
        break;

    case SD_CMD24:
        memcpy(&buffer[argPos], &request.cmd24.address, sizeof(request.cmd24.address));
        break;

    case SD_CMD25:
        memcpy(&buffer[argPos], &request.cmd25.address, sizeof(request.cmd25.address));
        break;

    default:
        return SD_SPI_UNSUPORTED_COMMAND_ERR_INT_STATUS;
    }

    buffer[FIELD_OFFSET(ReqLayout, cmd)] = (request.cmd ) | (0x1 << 6);
    buffer[FIELD_OFFSET(ReqLayout, crc)] = (crc7(buffer, 5) << 1) | 0x1;

    return SD_SPI_OK_INT_STATUS;
}

static SdSpiResult sdSpiDeserializeResp(uint8_t *buffer, SdSpiCmdResp *response, SdRespType respType)
{
    size_t r1Pos = FIELD_OFFSET(RespLayout, r1);
    size_t respPos = FIELD_OFFSET(RespLayout, resp);

    memset(response, 0x00, sizeof(SdSpiCmdResp));

    response->r1 = SD_R1_ADDRESS_ERROR | SD_R1_COMMAND_CRC_ERROR |
                   SD_R1_ERASE_RESET | SD_R1_ERASE_SEQUENSE_ERROR |
                   SD_R1_ILIGAL_COMMAND | SD_R1_IDLE_STATE | SD_R1_PARAMETER_ERROR;

    switch (respType) {
    case SD_RESPONSE_TYPE_R3:
        response->R3.cardCapacityStatys =
            BIT_MASK(buffer[respPos], SD_OCR_CCS_POS, SD_OCR_CCS_MASK) == 0
            ? OCR_CARD_CAPACITY_STATUS_SDSC
            : OCR_CARD_CAPACITY_STATUS_SDHC_SDXC;
        response->R3.switchingTo18V =
            BIT_MASK(buffer[respPos], SD_OCR_S18A_POS, SD_OCR_S18A_MASK) == 1;
        response->R3.cardPowerUp =
            BIT_MASK(buffer[respPos], SD_OCR_POWER_UP_POS, SD_OCR_POWER_UP_MASK) == 1;
        if (BIT_MASK(buffer[respPos], SD_OCR_v_27_28_POS, SD_OCR_v_27_28_MASK) == 1) {
            response->R3.vddVoltage = OCR_VDD_VOLTAGE_27_28;
        } else if (BIT_MASK(buffer[respPos], SD_OCR_v_28_29_POS, SD_OCR_v_28_29_MASK) == 1) {
            response->R3.vddVoltage = OCR_VDD_VOLTAGE_28_29;
        } else if (BIT_MASK(buffer[respPos], SD_OCR_v_29_30_POS, SD_OCR_v_29_30_MASK) == 1) {
            response->R3.vddVoltage = OCR_VDD_VOLTAGE_29_30;
        } else if (BIT_MASK(buffer[respPos], SD_OCR_v_30_31_POS, SD_OCR_v_30_31_MASK) == 1) {
            response->R3.vddVoltage = OCR_VDD_VOLTAGE_30_31;
        } else if (BIT_MASK(buffer[respPos], SD_OCR_v_31_32_POS, SD_OCR_v_31_32_MASK) == 1) {
            response->R3.vddVoltage = OCR_VDD_VOLTAGE_31_32;
        } else if (BIT_MASK(buffer[respPos], SD_OCR_v_32_33_POS, SD_OCR_v_32_33_MASK) == 1) {
            response->R3.vddVoltage = OCR_VDD_VOLTAGE_32_33;
        } else if (BIT_MASK(buffer[respPos], SD_OCR_v_33_34_POS, SD_OCR_v_33_34_MASK) == 1) {
            response->R3.vddVoltage = OCR_VDD_VOLTAGE_33_34;
        } else if (BIT_MASK(buffer[respPos], SD_OCR_v_34_35_POS, SD_OCR_v_34_35_MASK) == 1) {
            response->R3.vddVoltage = OCR_VDD_VOLTAGE_34_35;
        } else if (BIT_MASK(buffer[respPos], SD_OCR_v_35_36_POS, SD_OCR_v_35_36_MASK) == 1) {
            response->R3.vddVoltage = OCR_VDD_VOLTAGE_35_36;
        }
        break;

    case SD_RESPONSE_TYPE_R7:
        if (BIT_MASK(buffer[respPos], SD_R7_TEMPLATE_POS, SD_R7_TEMPLATE_MASK)
            != SD_CMD41_PATTERN_VALUE) {
            return SD_SPI_RESPONSE_R7_PATTERN_ERR_INT_STATUS;
        }
        response->R7.voltageAccepted =
            (buffer[respPos] & SD_R7_VHS) != 0;
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
        if (cmd = cmdMetaInfo[k].cmd) {
            return cmdMetaInfo[k].respType;
        }
    }

    return SD_RESPONSE_TYPE_CNT;
}

static SdSpiResult sdSpiWaiteBusy(SdSpiH *handler)
{
    uint8_t buff = 0x00;
    uint32_t startTime = handler->cb.sdSpiGetTimeMs();
    SdioSpiIntTrace *intTrace = (SdioSpiIntTrace *)handler->serviceBuff;

    while (handler->cb.sdSpiGetTimeMs() - startTime > SD_BUSY_TIMEOUTE) {
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
static SdSpiResult sdSpiCmdTransaction(SdSpiH *handler, SdSpiCmdReq request, SdSpiCmdResp *response)
{
    uint8_t reqBuff[sizeof(ReqLayout)];
    uint32_t rxCnt;
    SdioSpiIntTrace *intTrace = (SdioSpiIntTrace *)handler->serviceBuff;

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
    if (handler->cb.sdSpiSend(reqBuff, sizeof(ReqLayout)) == false) {
        return SD_SPI_RESULT_SEND_CB_RETURN_ERROR;
    }

    /*
     * Receive response
     */
    SdRespType respType = sdSpiGetRespType(request.cmd);
    size_t respSize = sdSpiGetRespSize(respType);
    uint8_t respBuff[respSize];

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
        if (handler->cb.sdSpiReceive(&respBuff[FIELD_OFFSET(RespLayout, r1)],
                                     FIELD_SIZE(RespLayout, r1)) == false) {
            return SD_SPI_RESULT_RECEIVE_CB_RETURN_ERROR;
        }
        if (respBuff[FIELD_OFFSET(RespLayout, r1)] != 0xFF) {
            break;
        }
    }

    /*
     * The card don't reply
     */
    if (respBuff[FIELD_OFFSET(RespLayout, r1)] == 0xFF) {
        return SD_SPI_RESULT_NO_RESPONSE_ERROR;
    }

    /*
     * Receive rest part of the reaponse
     */
    if (handler->cb.sdSpiReceive(&respBuff[FIELD_OFFSET(RespLayout, resp)],
                                 respSize - FIELD_SIZE(RespLayout, r1))
        == false) {
        return SD_SPI_RESULT_RECEIVE_CB_RETURN_ERROR;
    }

    if (handler->cb.sdSpiSetCsState(false) == true) {
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
    return (respType == SD_RESPONSE_TYPE_R1B)
           ? sdSpiWaiteBusy(handler)
           : SD_SPI_RESULT_OK;
}

static SdSpiResult sdSpiCardRun(SdSpiH *handler, SdCardVersion sdVersion)
{
    SdSpiCmdReq request;
    SdSpiCmdResp response;
    uint32_t startTime = handler->cb.sdSpiGetTimeMs();
    SdSpiResult result = SD_SPI_RESULT_OK;
    SdioSpiIntTrace *intTrace = (SdioSpiIntTrace *)handler->serviceBuff;

    while (handler->cb.sdSpiGetTimeMs() - startTime < SD_EXIT_IDLE_TIMEOUTE) {
        /***CMD55***/
        request.cmd = SD_CMD55;
        result = sdSpiCmdTransaction(handler, request, &response);
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
            request.cmd41.hcs = HCS_SDHC_SDXC;
        } else if (sdVersion == SD_CARD_VERSION_SD_VER_1) {
            request.cmd = SD_CMD41;
            request.cmd41.hcs = HCS_SDSC;
        } else {
            request.cmd = SD_CMD1;
        }
        result = sdSpiCmdTransaction(handler, request, &response);
        if (result != SD_SPI_RESULT_OK) {
            break;
        }
        if (response.r1 == 0x00) {
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
    bool initComplete = false;
    SdioSpiIntTrace *intTrace = (SdioSpiIntTrace *)handler->serviceBuff;

    if (handler == NULL) {
        return SD_SPI_RESULT_HANDLER_NULL_ERROR;
    }
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

    uint32_t serviceBuffSize = 0;
#ifdef ENABLE_ERROR_TRACE
    serviceBuffSize += sizeof(SdioSpiIntTrace);
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
    memset(transactionBuff, sizeof(transactionBuff), 0xFF);
    if (cb->sdSpiSetCsState(true) == false) {
        return SD_SPI_RESULT_SET_CS_CB_RETURN_ERROR;
    }
    if (cb->sdSpiSetSckFrq(SD_SPI_INITIAL_FRQ) == false) {
        return SD_SPI_RESULT_SET_FRQ_CB_RETURN_ERROR;
    }

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
    result = sdSpiCmdTransaction(handler, request, &response);
    if (result != SD_SPI_RESULT_OK) {
        return result;
    }
    if (response.r1 != SD_R1_IDLE_STATE) {
        intTrace->intStatus = SD_SPI_CMD55_REPLY_ERR_INT_STATUS;
        return SD_SPI_SET_IDLE_ERR_INT_STATUS;
    }

    request.cmd = SD_CMD8;
    request.cmd8.vhs = true;
    result = sdSpiCmdTransaction(handler, request, &response);
    if (result == SD_SPI_RESULT_OK
        && response.r1 == SD_R1_IDLE_STATE
        && response.R7.voltageAccepted == true) {

        /*
         * The card is SD Ver.2+
         */
        if (sdSpiCardRun(handler, SD_CARD_VERSION_SD_VER_2_PLUS) == SD_SPI_RESULT_OK) {

            /*
             * Test block length type
             */
            request.cmd = SD_CMD58;
            result = sdSpiCmdTransaction(handler, request, &response);
            if (result == SD_SPI_RESULT_OK && response.r1 == 0) {

                /*
                * If length type equal to Byte, we need to set block length equal to 512
                */
                if (response.R3.cardCapacityStatys == OCR_CARD_CAPACITY_STATUS_SDSC) {

                    /*
                    * Set block length equal to 512
                    */
                    request.cmd = SD_CMD16;
                    request.cmd16.blockLength = 512;
                    result = sdSpiCmdTransaction(handler, request, &response);
                    if (!(result == SD_SPI_RESULT_OK && response.r1 == 0)) {
                        /*
                        * Faile the cmd16 transaction
                        */
                        intTrace->intStatus = SD_SPI_SET_BLOCK_SIZE_ERR_INT_STATUS;
                        result = SD_SPI_RESULT_INTERNAL_ERROR;
                    }
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
             * Set block length equal to type 512
             */
            request.cmd = SD_CMD16;
            request.cmd16.blockLength = 512;
            result = sdSpiCmdTransaction(handler, request, &response);
            if (!(result == SD_SPI_RESULT_OK && response.r1 == 0)) {
                result = SD_SPI_RESULT_INIT_VER_1_SET_BLOCK_LEN_ERROR;
            }
        } else if (result == ){
            /*
             * In case of error, try init MMC Ver. 3
             */
        }

    }

    return result;
}
