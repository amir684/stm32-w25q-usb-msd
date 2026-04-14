/**
 * usb_device.c – אתחול ה-USB כ-MSC Device
 */
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_storage_if.h"

USBD_HandleTypeDef hUsbDeviceFS;

void MX_USB_DEVICE_Init(void)
{
    /* 1. אתחל core */
    USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);

    /* 2. רשום את ה-class (MSC) */
    USBD_RegisterClass(&hUsbDeviceFS, &USBD_MSC);

    /* 3. חבר את ה-storage callbacks (→ W25Q) */
    USBD_MSC_RegisterStorage(&hUsbDeviceFS, &USBD_Storage_Interface_fops_FS);

    /* 4. התחל לשדר! */
    USBD_Start(&hUsbDeviceFS);
}
