/**
 * stm32f4xx_it.c – מטפלי פסיקות (Interrupt Handlers)
 *
 * קובץ זה חיוני! בלעדיו:
 *   - HAL_Delay לא עובד (SysTick_Handler לא מעלה את uwTick)
 *   - USB לא עובד (OTG_FS_IRQHandler לא מוגדר כאן כי הוא ב-usbd_conf.c)
 */
#include "main.h"

/**
 * SysTick_Handler – called every 1ms.
 * Increments uwTick which HAL_Delay relies on.
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/**
 * HardFault_Handler – rapid LED blink = firmware crashed.
 * Visible signal: ~10 Hz toggle on PC13 (active-low LED).
 * If you see rapid blinking instead of the 3-blink pattern,
 * a HardFault occurred (stack overflow, null pointer, etc.).
 */
void HardFault_Handler(void)
{
    __disable_irq();
    /* Ensure GPIOC clock is on and PC13 is output */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER = (GPIOC->MODER & ~(3U << 26)) | (1U << 26); /* PC13 output */
    while (1)
    {
        GPIOC->ODR ^= (1U << 13);                 /* toggle LED */
        for (volatile uint32_t i = 0; i < 84000U; i++); /* ~1ms at 84MHz */
    }
}

/* Alias the other fault handlers to HardFault for visibility */
void MemManage_Handler(void)  __attribute__((alias("HardFault_Handler")));
void BusFault_Handler(void)   __attribute__((alias("HardFault_Handler")));
void UsageFault_Handler(void) __attribute__((alias("HardFault_Handler")));

