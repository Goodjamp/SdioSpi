#ifndef __SD_SPI_H__
#define __SD_SPI_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
    SD_SPI_RESULT_OK,
    SD_SPI_RESULT_HANDLER_NULL_ERROR,
    SD_SPI_RESULT_CB_NULL_ERROR,
    SD_SPI_RESULT_SPI_SET_FRQ_CB_NULL_ERROR,
    SD_SPI_RESULT_SPI_SET_CS_CB_NULL_ERROR,
    SD_SPI_RESULT_SPI_SEND_CB_NULL_ERROR,
    SD_SPI_RESULT_SPI_RECEIVE_CB_NULL_ERROR,
    SD_SPI_RESULT_SPI_GET_TIME_CB_NULL_ERROR,
    SD_SPI_RESULT_SPI_SET_FRQ_CB_RETURN_ERROR,
    SD_SPI_RESULT_SPI_SET_CS_CB_RETURN_ERROR,
    SD_SPI_RESULT_SPI_SEND_CB_RETURN_ERROR,
    SD_SPI_RESULT_SPI_RECEIVE_CB_RETURN_ERROR,
    SD_SPI_RESULT_INTERNAL_ERROR,
    SD_SPI_RESULT_NO_RESPONSE_ERROR,
    SD_SPI_RESULT_INIT_SET_IDLE_ERROR,
    SD_SPI_RESULT_RESPONSE_R7_PATTERN_ERROR,
    SD_SPI_RESULT_CMD55_NO_REPLY_ERROR,
    SD_SPI_RESULT_CMD55_REPLY_ERROR,
    SD_SPI_RESULT_CMD41_PROCESSING_ERROR,
    SD_SPI_RESULT_CMD41_REPLY_ERROR,
    SD_SPI_RESULT_SD_VER2_PLUS_INIT_ERROR,
    SD_SPI_RESULT_RUN_TIMEOUTE_ERROR,
    SD_SPI_BUSY_TIMEOUTE_ERROR,
} SdSpiResult;

typedef enum {
    SD_CARD_VERSION_SD_VER_1,
    SD_CARD_VERSION_SD_VER_2_PLUS,
    SD_CARD_VERSION_MMC_VER_2
} SdCardVersion;

typedef struct {
    SdCardVersion version;
} SdSpiMetaInformation;

typedef struct {
    bool (*sdSpiSend)(uint8_t *data, size_t dataLength);
    bool (*sdSpiReceive)(uint8_t *data, size_t dataLength);
    bool (*sdSpiSetCsState)(bool set);
    bool (*sdSpiSetSckFrq)(bool hight);
    uint32_t (*sdSpiGetTimeMs)(void);
} SdSpiCb;

typedef struct {
    SdSpiCb cb;
    int32_t internalError;
    uint32_t dataBlockSize;
    SdSpiMetaInformation metaInformation;
} SdSpiH;

SdSpiResult sdSpiInit(SdSpiH *handler, const SdSpiCb *cb);
SdSpiResult sdSpiGetMetaInformation(SdSpiH *handler, SdSpiMetaInformation metaInformation );
SdSpiResult sdSpiGetInternalError(SdSpiH *handler);
SdSpiResult sdSpiReceive(SdSpiH *handler, uint8_t *data, size_t dataLength);
SdSpiResult sdSpiWrite(SdSpiH *handler, uint8_t *data, size_t dataLength);


#endif