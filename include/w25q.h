#pragma once

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* ---- פינים של Chip Select ---- */
#define W25Q_CS_PORT   GPIOA
#define W25Q_CS_PIN    GPIO_PIN_4

/* ── Single configuration knob ────────────────────────────────────────────
   Change FLASH_SIZE_MB to match your chip and everything else follows.
   Common W25Qxx sizes: 1, 2, 4, 8, 16, 32, 64 (MB)
   Page and sector geometry is identical across the entire W25Qxx family.  */
#define FLASH_SIZE_MB  8U

/* Derived – do not edit below this line */
#define W25Q_CAPACITY_BYTES  ((FLASH_SIZE_MB) * 1024UL * 1024UL)
#define W25Q_PAGE_SIZE       256U    /* write unit – all W25Qxx models */
#define W25Q_SECTOR_SIZE     4096U   /* erase unit – all W25Qxx models */
#define W25Q_SECTOR_COUNT    (W25Q_CAPACITY_BYTES / W25Q_SECTOR_SIZE)


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
