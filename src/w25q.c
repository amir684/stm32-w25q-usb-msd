/**
 * w25q.c – דרייבר SPI לזיכרון W25Qxx (נבדק עם W25Q64)
 *
 * חיבורים (Black Pill):
 *   PA5 = SCK
 *   PA6 = MISO
 *   PA7 = MOSI
 *   PA4 = CS (GPIO רגיל)
 */
#include "w25q.h"

/* ---- פקודות W25Q ---- */
#define CMD_READ         0x03
#define CMD_PAGE_PROG    0x02
#define CMD_SECTOR_ERASE 0x20   /* מחיקת 4KB */
#define CMD_WREN         0x06   /* Write Enable */
#define CMD_READ_SR1     0x05
#define CMD_READ_ID      0x9F

#define SR1_BUSY         0x01

/* ---- מאקרו CS ---- */
#define CS_LOW()  HAL_GPIO_WritePin(W25Q_CS_PORT, W25Q_CS_PIN, GPIO_PIN_RESET)
#define CS_HIGH() HAL_GPIO_WritePin(W25Q_CS_PORT, W25Q_CS_PIN, GPIO_PIN_SET)

static SPI_HandleTypeDef *_spi;

/* ============================================================ */

void W25Q_Init(SPI_HandleTypeDef *hspi)
{
    _spi = hspi;
    CS_HIGH();
}

/* ---- פונקציות עזר פנימיות ---- */

static void spi_write(const uint8_t *data, uint32_t len)
{
    HAL_SPI_Transmit(_spi, (uint8_t *)data, (uint16_t)len, HAL_MAX_DELAY);
}

static void spi_read(uint8_t *data, uint32_t len)
{
    HAL_SPI_Receive(_spi, data, (uint16_t)len, HAL_MAX_DELAY);
}

static inline void send_addr_cmd(uint8_t cmd, uint32_t addr)
{
    uint8_t buf[4] = {
        cmd,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };
    spi_write(buf, 4);
}

static void write_enable(void)
{
    uint8_t cmd = CMD_WREN;
    CS_LOW();
    spi_write(&cmd, 1);
    CS_HIGH();
}

/* ============================================================ */

uint8_t W25Q_ReadSR1(void)
{
    uint8_t cmd = CMD_READ_SR1, sr;
    CS_LOW();
    spi_write(&cmd, 1);
    spi_read(&sr, 1);
    CS_HIGH();
    return sr;
}

void W25Q_WaitBusy(void)
{
    while (W25Q_ReadSR1() & SR1_BUSY);
}

uint32_t W25Q_ReadID(void)
{
    uint8_t cmd = CMD_READ_ID, buf[3];
    CS_LOW();
    spi_write(&cmd, 1);
    spi_read(buf, 3);
    CS_HIGH();
    return ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
}

/* ============================================================ */

void W25Q_ReadData(uint32_t addr, uint8_t *buf, uint32_t len)
{
    CS_LOW();
    send_addr_cmd(CMD_READ, addr);
    spi_read(buf, len);
    CS_HIGH();
}

void W25Q_WritePage(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    /* len <= 256, לא חוצה גבול עמוד */
    W25Q_WaitBusy();
    write_enable();
    CS_LOW();
    send_addr_cmd(CMD_PAGE_PROG, addr);
    spi_write(buf, len);
    CS_HIGH();
    W25Q_WaitBusy();
}

void W25Q_EraseSector(uint32_t sector_addr)
{
    /* sector_addr חייב להיות מיושר ל-4KB */
    W25Q_WaitBusy();
    write_enable();
    CS_LOW();
    send_addr_cmd(CMD_SECTOR_ERASE, sector_addr);
    CS_HIGH();
    W25Q_WaitBusy();
}
