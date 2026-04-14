/**
 * stm32f4xx_hal_msp.c – אתחול חומרה (MSP = MCU Support Package)
 *
 * כאן מגדירים GPIO ו-שעונים לכל פריפרי.
 * ה-USB (PCD) מוגדר ב-usbd_conf.c.
 */
#include "main.h"

/**
 * HAL_SPI_MspInit – נקרא מתוך HAL_SPI_Init()
 * SPI1 על Black Pill:
 *   PA5 = SCK
 *   PA6 = MISO
 *   PA7 = MOSI
 *   PA4 = CS (GPIO רגיל, מנוהל ידנית ב-w25q.c)
 */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance != SPI1) return;

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA5=SCK, PA6=MISO, PA7=MOSI  →  Alternate Function SPI1 */
    GPIO_InitStruct.Pin       = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance != SPI1) return;
    __HAL_RCC_SPI1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
}
