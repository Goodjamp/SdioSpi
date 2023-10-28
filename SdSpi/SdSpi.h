#ifndef __SD_SPI_H__
#define __SD_SPI_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
    SD_SPI_RESULT_OK,
    SD_SPI_RESULT_HANDLER_NULL_ERROR,
    SD_SPI_RESULT_CB_NULL_ERROR,
    SD_SPI_RESULT_SET_FRQ_CB_NULL_ERROR,
    SD_SPI_RESULT_SET_CS_CB_NULL_ERROR,
    SD_SPI_RESULT_SEND_CB_NULL_ERROR,
    SD_SPI_RESULT_RECEIVE_CB_NULL_ERROR,
    SD_SPI_RESULT_GET_TIME_CB_NULL_ERROR,
    SD_SPI_RESULT_MALLOC_CB_NULL_ERROR,
    SD_SPI_RESULT_SET_FRQ_CB_RETURN_ERROR,
    SD_SPI_RESULT_SET_CS_CB_RETURN_ERROR,
    SD_SPI_RESULT_SEND_CB_RETURN_ERROR,
    SD_SPI_RESULT_RECEIVE_CB_RETURN_ERROR,
    SD_SPI_RESULT_MALLOC_CB_RERTURN_NULL_ERROR,
    SD_SPI_RESULT_INTERNAL_ERROR,
    SD_SPI_RESULT_NO_RESPONSE_ERROR,
    SD_SPI_RESULT_DATA_NULL_ERROR,
    SD_SPI_RESULT_DATA_LENGTH_ZERO_ERROR,
    SD_SPI_RESULT_NOT_INIT_ERROR,
} SdSpiResult;

typedef enum {
    SD_CARD_VERSION_SD_VER_1,
    SD_CARD_VERSION_SD_VER_2_PLUS,
    SD_CARD_VERSION_MMC_VER_2
} SdCardVersion;

typedef enum {
    SD_CARD_CAPCITY_STANDART,
    SD_CARD_CAPCITY_HIGHT,    // SDHC
    SD_CARD_CAPCITY_EXTENDED, // SDХC
} SdCardCapacity;

typedef struct {
    SdCardVersion version;
    SdCardCapacity capcity;
} SdSpiMetaInformation;

typedef struct {
    bool (*sdSpiSend)(uint8_t *data, size_t dataLength);
    bool (*sdSpiReceive)(uint8_t *data, size_t dataLength);
    bool (*sdSpiSetCsState)(bool set);
    bool (*sdSpiSetSckFrq)(bool hight);
    uint32_t (*sdSpiGetTimeMs)(void);
    uint8_t *(*sdSpiMalloc)(uint32_t size);
} SdSpiCb;

typedef struct {
    SdSpiCb cb;
    SdSpiMetaInformation metaInformation;

    /*
     * The serviceBuff is used to save the internal errors.
     * Also this buffer could be used as a traceBuffer
     */
    uint8_t *serviceBuff;

    uint8_t *transactionBuffer;
} SdSpiH;

SdSpiResult sdSpiInit(SdSpiH *handler, const SdSpiCb *cb);
SdSpiResult sdSpiGetMetaInformation(SdSpiH *handler, SdSpiMetaInformation metaInformation );
SdSpiResult sdSpiGetInternalError(SdSpiH *handler);
SdSpiResult sdSpiReceive(SdSpiH *handler, uint8_t *data, size_t dataLength);
SdSpiResult sdSpiWrite(SdSpiH *handler, uint8_t *data, size_t dataLength);


#endif