#pragma once

#include "stm32f4xx_hal.h"
#include "usbd_def.h"

#define DEVICE_FS   0U   /* USB Full-Speed instance id */

extern USBD_HandleTypeDef hUsbDeviceFS;

void MX_USB_DEVICE_Init(void);
