#pragma once

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* ---- פינים של Chip Select ---- */
#define W25Q_CS_PORT   GPIOA
#define W25Q_CS_PIN    GPIO_PIN_4

/* ---- קבועים של W25Q64 ---- */
#define W25Q64_CAPACITY_BYTES   (8UL * 1024 * 1024)   /* 8MB */
#define W25Q64_PAGE_SIZE        256U                   /* bytes */
#define W25Q64_SECTOR_SIZE      4096U                  /* bytes – גודל מחיקה */
#define W25Q64_SECTOR_COUNT     (W25Q64_CAPACITY_BYTES / W25Q64_SECTOR_SIZE)

/* ---- API ---- */
void     W25Q_Init(SPI_HandleTypeDef *hspi);
uint32_t W25Q_ReadID(void);
void     W25Q_WaitBusy(void);

/* קרא bytes רצוף מכל כתובת */
void W25Q_ReadData(uint32_t addr, uint8_t *buf, uint32_t len);

/* כתוב עמוד (max 256 bytes, לא חוצה גבול עמוד) */
void W25Q_WritePage(uint32_t addr, const uint8_t *buf, uint32_t len);

/* מחק סקטור של 4KB */
void W25Q_EraseSector(uint32_t sector_addr);
