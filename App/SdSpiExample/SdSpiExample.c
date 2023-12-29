#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_it.h"

#include "SEGGER_RTT.h"

#include "Bsp.h"
#include "services.h"
#include "Spi.h"
#include "SdSpi.h"
#include "DebugServices.h"

static bool txComplete;
static bool rxComplete;
static SdSpiH sdSpiHandler;

#define PRINT_LOG(FORMAT, ...)    {       \
    char logStr[128];                     \
    sprintf(logStr, FORMAT, __VA_ARGS__); \
    SEGGER_RTT_WriteString(0, logStr);    \
}

/*
 *------------------------  SPI CB   ------------------------
 */
static void spiRxCompleteCb(SpiResult result)
{
    rxComplete = true;
}

static void spiTxCompleteCb(SpiResult result)
{
    debugServicesPinSet(DebugPin2);
    txComplete = true;
    debugServicesPinClear(DebugPin2);
}

/*
 *------------------------  SD CB   ------------------------
 */

static bool sdSpiSendCb(uint8_t *data, size_t dataLength)
{
    if (dataLength == 0) {
        return true;
    }

    debugServicesPinSet(DebugPin1);
    txComplete = false;
    spiTx(SPI_ETH, data, dataLength);
    while (txComplete == false){}

    debugServicesPinClear(DebugPin1);
    return true;
}

static bool sdSpiReceiveCb(uint8_t *data, size_t dataLength)
{
    if (dataLength == 0) {
        return true;
    }

    debugServicesPinSet(DebugPin3);
    rxComplete = false;
    spiRx(SPI_ETH, data, dataLength);
    while (rxComplete == false){}

    debugServicesPinClear(DebugPin3);

    return true;
}

static bool sdSpiSetCsStateCb(bool set)
{
    spiCsControl(SPI_ETH, set);
    /*
    if (set == true) {
        LL_GPIO_SetOutputPin(ETH_SPI_GPIO_CS_PORT, ETH_SPI_GPIO_CS_PIN);
    } else {
        LL_GPIO_ResetOutputPin(ETH_SPI_GPIO_CS_PORT, ETH_SPI_GPIO_CS_PIN);
    }
    */

    return true;
}

static bool sdSpiSetSckFrqCb(bool hight)
{
    //LL_GPIO_SetOutputPin(ETH_SPI_GPIO_CS_PORT, ETH_SPI_GPIO_CS_PIN);

    return true;
}

static uint32_t sdSpiGetTimeMsCb(void)
{
    return getTickCount();
}

static uint8_t *sdSpiMallocCb(uint32_t size)
{
    static uint8_t memBuff[1024];

    return memBuff;
}

static void sdSpiInitCs(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {
        .OutputType = LL_GPIO_OUTPUT_PUSHPULL,
        .Pull = LL_GPIO_PULL_NO,
        .Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH,
        .Pin = ETH_SPI_GPIO_CS_PIN,
        .Mode = LL_GPIO_MODE_OUTPUT,
        .Alternate = LL_GPIO_AF_5,
    };

    servicesEnablePerephr(ETH_SPI_GPIO_CS_PORT);
    LL_GPIO_Init(ETH_SPI_GPIO_CS_PORT, &GPIO_InitStruct);
}

static void sdSpiInitilisation(void)
{
    SpiResult spiResult;
    SdSpiResult sdSpiResult;
    SpiSettings settings = {
        .eth.speed = 400000,
    };
    SpiCb spiCb = {
        .eth={spiRxCompleteCb, spiTxCompleteCb}
    };
    SdSpiCb sdSpiCb = {
        .sdSpiSend = sdSpiSendCb,
        .sdSpiReceive = sdSpiReceiveCb,
        .sdSpiSetCsState = sdSpiSetCsStateCb,
        .sdSpiSetSckFrq = sdSpiSetSckFrqCb,
        .sdSpiGetTimeMs = sdSpiGetTimeMsCb,
        .sdSpiMalloc = sdSpiMallocCb,
    };

    sdSpiInitCs();
    spiResult = spiInit(SPI_ETH, settings, spiCb);
    PRINT_LOG("Spi Ini result: %u\n", spiResult);

    sdSpiResult = sdSpiInit(&sdSpiHandler, &sdSpiCb);
    PRINT_LOG("Sd Spi Ini result: %u\n", sdSpiResult);

}

#define RX_DATA_SIZE    (512 * 32)
uint8_t sdCardData[RX_DATA_SIZE];
void  sdSpiExampleRun(void)
{
    SdSpiResult result;
    sdSpiInitilisation();

    /*
     * Test receive 512 bytes
     */
    memset(sdCardData, 0, sizeof(sdCardData));
    result = sdSpiRead(&sdSpiHandler, (uint32_t )0, sdCardData, 1);
    PRINT_LOG("Sd receive 512 bytes result: %u\n", result);

    for (uint32_t k = 8192; k < 8192 + 32; k++) {
        memset(sdCardData, 0, sizeof(sdCardData));
        result = sdSpiRead(&sdSpiHandler, k, sdCardData, 1);
    }

    /*
     * Test receive 8192 bytes
     */
    memset(sdCardData, 0, sizeof(sdCardData));
    result = sdSpiRead(&sdSpiHandler, (uint32_t )(8192), sdCardData, 32);
    PRINT_LOG("Sd receive 2048 bytes result: %u\n", result);

}