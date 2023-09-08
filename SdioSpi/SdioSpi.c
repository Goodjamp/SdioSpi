#include "SdioSpi.h"


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
 * | CMD8   | *3                     | R7  | No  | SEND_IF_COND             | For only SDC V2. Check voltage range                |
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
 * | CMD24  | Address[31:0]          | R1  | Yes | WRITE_BLOCK              | Write a block                                      |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD25  | Address[31:0]          | R1  | Yes | WRITE_MULTIPLE_BLOCK     | Write multiple blocks                               |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD55  | None(0)                | R1  | No  | APP_CMD                  | Leading command of ACMD<n> command                  |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+
 * | CMD58  | None(0)                | R3  | No  | READ_OCR                 | Read OCR                                            |
 * +--------+------------------------+-----+-----+--------------------------+-----------------------------------------------------+

 */
#define SDIO_CMD0     0
#define SDIO_CMD1     0
#define SDIO_CMD41    0
#define SDIO_CMD8     0
#define SDIO_CMD9     0
#define SDIO_CMD10    0
#define SDIO_CMD12    0
#define SDIO_CMD16    0
#define SDIO_CMD17    0
#define SDIO_CMD18    0
#define SDIO_CMD23    0
#define SDIO_CMD24    0
#define SDIO_CMD25    0
#define SDIO_CMD55    0
#define SDIO_CMD58    0

SpioSpiResult sdioSpiInit(SdioSpiH *handler, const SdioSpiCb *cb)
{
    if (handler == NULL) {
        return SDIO_SPI_RESULT_HANDLER_NULL_ERROR;
    }
    if (cb == NULL) {
        return SDIO_SPI_RESULT_CB_NULL_ERROR;
    }
    if (cb->spiWriteCb == NULL) {
        return SDIO_SPI_RESULT_SPI_WRITE_CB_NULL_ERROR;
    }
    if (cb->spiReadCb == NULL) {
        return SDIO_SPI_RESULT_SPI_READ_CB_NULL_ERROR;
    }
    if (cb->spiSetCsStateCb == NULL) {
        return SDIO_SPI_RESULT_SPI_SET_CS_CB_NULL_ERROR;
    }
    if (cb->spiSetSckFrq == NULL) {
        return SDIO_SPI_RESULT_SPI_SET_FRQ_CB_NULL_ERROR;
    }
    
    return SDIO_SPI_RESULT_OK;
}
