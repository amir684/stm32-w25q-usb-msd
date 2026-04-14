#pragma once

/* ========== מודולי HAL שפעילים ========== */
#define HAL_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_PCD_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED

/* ========== Oscillator ========== */
#define HSE_VALUE    25000000U   /* Black Pill HSE = 25MHz */
#define HSI_VALUE    16000000U
#define HSE_STARTUP_TIMEOUT   100U
#define HSI_STARTUP_TIMEOUT   5000U
#define LSI_VALUE   32000U
#define LSE_VALUE   32768U

/* ========== SysTick ========== */
/* חייב להיות נמוך מ-USB (5) כדי ש-HAL_Delay יעבוד תמיד.
   ב-ARM: מספר נמוך יותר = עדיפות גבוהה יותר. */
#define  TICK_INT_PRIORITY   0x00U

/* ========== includes של כל מודול ========== */
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_pwr.h"
#include "stm32f4xx_hal_spi.h"
#include "stm32f4xx_hal_pcd.h"
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_dma.h"

/* check values */
#define assert_param(expr) ((void)0U)
