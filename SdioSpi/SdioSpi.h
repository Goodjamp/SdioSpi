#ifndef __SDIO_SPI_H__
#define __SDIO_SPI_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
    SDIO_SPI_RESULT_OK,
    SDIO_SPI_RESULT_HANDLER_NULL_ERROR,
    SDIO_SPI_RESULT_CB_NULL_ERROR,
    SDIO_SPI_RESULT_SPI_WRITE_CB_NULL_ERROR,
    SDIO_SPI_RESULT_SPI_READ_CB_NULL_ERROR,
    SDIO_SPI_RESULT_SPI_SET_CS_CB_NULL_ERROR,
    SDIO_SPI_RESULT_SPI_SET_FRQ_CB_NULL_ERROR,
} SpioSpiResult;

typedef struct {
    bool (*spiWriteCb)(uint8_t *data, size_t dataLength);
    bool (*spiReadCb)(uint8_t *data, size_t dataLength);
    bool (*spiSetCsStateCb)(bool set);
    bool (*spiSetSckFrq)(bool hight);
} SdioSpiCb;

typedef struct {
    SdioSpiCb cb;
    int32_t internalError;
} SdioSpiH;

SpioSpiResult sdioSpiInit(SdioSpiH *handler, const SdioSpiCb *cb);
SpioSpiResult sdioSpiGetInfo(SdioSpiH *handler);
SpioSpiResult sdioSpiGetIntarnalError(SdioSpiH *handler);
SpioSpiResult sdioSpiRead(SdioSpiH *handler, uint8_t *data, size_t dataLength);
SpioSpiResult sdioSpiWrite(SdioSpiH *handler, uint8_t *data, size_t dataLength);


#endif