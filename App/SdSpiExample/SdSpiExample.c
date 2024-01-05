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

#include "SdSpiInternal.h"

static volatile bool txComplete;
static volatile bool rxComplete;
static SdSpiH sdSpiHandler;

#define PRINT_LOG(FORMAT, ...)    {       \
    char logStr[128];                     \
    sprintf(logStr, FORMAT, __VA_ARGS__); \
    SEGGER_RTT_WriteString(0, logStr);    \
}

static const char writeData[] = "He was not the Model Boy of the village. He knew the model boy very well though—and loathed him. Within two minutes, or even less, he had forgotten all his troubles. Not because his troubles were one whit less heavy and bitter to him than a man’s are to a man, but because a new and powerful interest bore them down and drove them out of his mind for the time—just as men’s misfortunes are forgotten in the excitement of new enterprises. This new interest was a valued novelty in whistling, which he had just acquired from a negro, and he was suffering to practise it undisturbed. It consisted in a peculiar bird-like turn, a sort of liquid warble, produced by touching the tongue to the roof of the mouth at short intervals in the midst of the music—the reader probably remembers how to do it, if he has ever been a boy. Diligence and attention soon gave him the knack of it, and he strode down the street with his mouth full of harmony and his soul full of gratitude. He felt much as an astronomer feels who has discovered a new planet—no doubt, as far as strong, deep, unalloyed pleasure is concerned, the advantage was with the boy, not the astronomer.The summer evenings were long. It was not dark, yet. Presently Tom checked his whistle. A stranger was before him—a boy a shade larger than himself. A new-comer of any age or either sex was an impressive curiosity in the poor little shabby village of St. Petersburg. This boy was well dressed, too—well dressed on a week-day. This was simply astounding. His cap was a dainty thing, his close-buttoned blue cloth roundabout was new and natty, and so were his pantaloons. He had shoes on—and it was only Friday. He even wore a necktie, a bright bit of ribbon. He had a citified air about him that ate into Tom’s vitals. The more Tom stared at the splendid marvel, the higher he turned up his nose at his finery and the shabbier and shabbier his own outfit seemed to him to grow. Neither boy spoke. If one moved, the other moved—but only sidewise, in a circle; they kept face to face and eye to eye all the time. Finally Tom said:";

/*
 *------------------------  SPI CB   ------------------------
 */
static void spiRxCompleteCb(SpiResult result)
{
    rxComplete = true;
}

static void spiTxCompleteCb(SpiResult result)
{
    txComplete = true;
}

/*
 *------------------------  SD CB   ------------------------
 */

static bool sdSpiSendCb(uint8_t *data, size_t dataLength)
{
    if (dataLength == 0) {
        return true;
    }

    txComplete = false;
    spiTx(SPI_ETH, data, dataLength);
    while (txComplete == false){}

    return true;
}

static bool sdSpiReceiveCb(uint8_t *data, size_t dataLength)
{
    if (dataLength == 0) {
        return true;
    }

    rxComplete = false;
    spiRx(SPI_ETH, data, dataLength);
    while (rxComplete == false){}

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
uint8_t csdReg[SD_SPI_CSD_BYTES];
uint8_t cidReg[SD_SPI_CID_BYTES];

void  sdSpiExampleRun(void)
{
    SdSpiResult result;
    SdSpiMetaInformation metaInformation;

    sdSpiInitilisation();

    sdSpiGetMetaInformation(&sdSpiHandler, &metaInformation);
    PRINT_LOG("Sd capacity: %u\n", (unsigned int)metaInformation.capcityMb);

    /*
     * Test receive 1 LBA (512 bytes)
     */
    memset(sdCardData, 0, sizeof(sdCardData));
    result = sdSpiRead(&sdSpiHandler, (uint32_t )0, sdCardData, 1);
    PRINT_LOG("Sd receive 512 bytes result: %u\n", result);

    for (uint32_t k = 8192; k < 8192 + 32; k++) {
        memset(sdCardData, 0, sizeof(sdCardData));
        result = sdSpiRead(&sdSpiHandler, k, sdCardData, 1);
    }

    /*
     * Test receive multiple LBA by the 8192 LBA address (8192 * 512 abs address)
     */
    memset(sdCardData, 0, sizeof(sdCardData));
    result = sdSpiRead(&sdSpiHandler, (uint32_t )(8192), sdCardData, 32);
    PRINT_LOG("Sd receive 2048 bytes result: %u\n", result);





    /*
     * Test send 2 LBA by the 0 LBA address
     */
    result = sdSpiWrite(&sdSpiHandler, 0, (uint8_t *)&writeData[512 * 2], 2);
    PRINT_LOG("Sd Write 512*2 bytes result: %u\n", result);

    /*
     * Test receive 2 LBA (512 bytes)
     */
    memset(sdCardData, 0, sizeof(sdCardData));
    result = sdSpiRead(&sdSpiHandler, (uint32_t )0, sdCardData, 2);
    PRINT_LOG("Sd receive 512 bytes result: %u\n", result);

    if (memcmp(sdCardData, &writeData[512 * 2], 2 * 512) == 0) {
        PRINT_LOG("Sd write/read Ok%c\n", ' ');
    } else {
        PRINT_LOG("Sd write/read ERROR%c\n", ' ');
    }




    /*
     * Test send 2 LBA by the 0 LBA address
     */
    result = sdSpiWrite(&sdSpiHandler, 0, (uint8_t *)&writeData[0], 2);
    PRINT_LOG("Sd Write 512*2 bytes result: %u\n", result);

    /*
     * Test receive 2 LBA (512 bytes)
     */
    memset(sdCardData, 0, sizeof(sdCardData));
    result = sdSpiRead(&sdSpiHandler, (uint32_t )0, sdCardData, 2);
    PRINT_LOG("Sd receive 512 bytes result: %u\n", result);

    if (memcmp(sdCardData, &writeData[0], 2 * 512) == 0) {
        PRINT_LOG("Sd write/read Ok%c\n", ' ');
    } else {
        PRINT_LOG("Sd write/read ERROR%c\n", ' ');
    }
}