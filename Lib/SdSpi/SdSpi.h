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
    SD_SPI_RESULT_DATA_NULL_ERROR,
    SD_SPI_RESULT_DATA_LENGTH_ZERO_ERROR,

    SD_SPI_RESULT_INTERNAL_ERROR,
    
    /*
     * Card don't reply
     */
    SD_SPI_RESULT_NO_RESPONSE_ERROR,
    
    /*
     * The R1 response != 0
     */
    SD_SPI_RESULT_RESPONSE_ERROR,

    /*
     * Receive the error token to the Read command
     */
    SD_SPI_RESULT_RECEIVE_ERROR,

    /*
     * Receive the error to the Write command
     */
    SD_SPI_RESULT_WRITE_ERROR,

    SD_SPI_RESULT_UNKNOWN_ERROR,
} SdSpiResult;

typedef enum {
    SD_CARD_VERSION_SD_VER_1,
    SD_CARD_VERSION_SD_VER_2_PLUS,
    SD_CARD_VERSION_MMC_VER_2
} SdCardVersion;

typedef enum {
    SD_CARD_CAPCITY_STANDART, // SD   standart Capacity
    SD_CARD_CAPCITY_HIGHT,    // SDHC hight capacity
    SD_CARD_CAPCITY_EXTENDED, // SDXC eXtended capacity (UHS-I and UHS-II )
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
     * the information aboit LBA (1 byte / 512 bytes)
     */
    uint16_t lba;

    /*
     * The serviceBuff is used to save the internal errors.
     * Also this buffer could be used as a traceBuffer
     */
    uint8_t *serviceBuff;

    uint8_t *transactionBuffer;
} SdSpiH;

SdSpiResult sdSpiInit(SdSpiH *handler, const SdSpiCb *cb);

/**
 * @brief read data from the card
 * @param[in] handler - the handler of the SdCard item
 * @param[in] address - the address of the target sector. The absolute address is calculated as (address * 512)
 * @param[in] data - the buffer for the read data. The buffer size must be (data length * 512)
 * @param[in] dataLength - the number of logical blocks to read. The   logical block size equal to 512 bytes
 */
SdSpiResult sdSpiRead(SdSpiH *handler, uint32_t address, uint8_t *data, size_t dataLength);
SdSpiResult sdSpiWrite(SdSpiH *handler, uint32_t address, uint8_t *data, size_t dataLength);

/*
 * Services
 */
SdSpiResult sdSpiGetMetaInformation(SdSpiH *handler, SdSpiMetaInformation metaInformation );
SdSpiResult sdSpiGetInternalError(SdSpiH *handler);
#endif