/**
 * main.c – STM32F401 USB Mass Storage Device
 *
 * W25Q64 SPI Flash נחשף כ-כונן USB של 8MB.
 *
 * חיבורים:
 *   USB:  PA11=DM, PA12=DP
 *   SPI1: PA5=SCK, PA6=MISO, PA7=MOSI
 *   CS:   PA4   (active-low)
 *   LED:  PC13  (active-low)
 *   HSE:  25MHz crystal (Black Pill)
 *
 * קוד LED:
 *   3 פעימות קצרות + הפסקה = USB init עבר, מחכה לחיבור
 */
#include "main.h"
#include "w25q.h"
#include "usb_device.h"

SPI_HandleTypeDef hspi1;

static void SystemClock_Config(void);
static void MX_SPI1_Init(void);
static void MX_GPIO_Init(void);

/* ---- LED helpers (active-low, PC13) ---- */
#define LED_ON()  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET)
#define LED_OFF() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET)

int main(void)
{
    /* וודא ש-VTOR מצביע לקוד שלנו ולא ל-DFU bootloader */
    SCB->VTOR = FLASH_BASE;

    HAL_Init();
    SystemClock_Config();

    /* ---- LED init (אחרי clock config כי HAL_Delay עובד) ---- */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    {
        GPIO_InitTypeDef g = {GPIO_PIN_13, GPIO_MODE_OUTPUT_PP,
                              GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0};
        HAL_GPIO_Init(GPIOC, &g);
    }
    LED_ON();

    MX_GPIO_Init();
    MX_SPI1_Init();
    W25Q_Init(&hspi1);

    MX_USB_DEVICE_Init();

    /* ---- לולאה ראשית: 3 פעימות + הפסקה = מחכה לחיבור USB ---- */
    while (1)
    {
        for (int i = 0; i < 3; i++) {
            LED_OFF(); HAL_Delay(120);
            LED_ON();  HAL_Delay(120);
        }
        HAL_Delay(800);
    }
}

/* ============================================================
   שעון: HSE 25MHz → PLL → 84MHz SYSCLK, 48MHz USB
   USB Full Speed דורש ±0.25% — HSE מספק זאת בדיוק.
   VCO  = 25 / 25 * 336 = 336MHz
   SYSCLK = 336 / 4  = 84MHz
   USB    = 336 / 7  = 48.000MHz  ← מדויק
   ============================================================ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 336;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ       = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

/* ============================================================
   GPIO – CS pin של W25Q (PA4), HIGH = לא נבחר
   ============================================================ */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();

    HAL_GPIO_WritePin(W25Q_CS_PORT, W25Q_CS_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = W25Q_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(W25Q_CS_PORT, &GPIO_InitStruct);
}

/* ============================================================
   SPI1 – Full Duplex Master
   PCLK2 = 84MHz, prescaler /4 → SPI clock = 21MHz
   ============================================================ */
static void MX_SPI1_Init(void)
{
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
        Error_Handler();
}

/* ============================================================
   Error Handler – LED מהבהב מהיר מאוד = משהו כשל
   ============================================================ */
void Error_Handler(void)
{
    __disable_irq();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        for (volatile uint32_t i = 0; i < 400000UL; i++);
    }
}
