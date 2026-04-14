/**
 * usbd_desc.c – מתארים (Descriptors) של התקן USB
 * VID/PID: ST's generic MSC (0483:5720)
 */
#include "usbd_desc.h"
#include "usbd_core.h"
#include "usbd_conf.h"

/* ---- זיהוי ---- */
#define USBD_VID             0x0483U  /* ST Microelectronics */
#define USBD_PID             0x5720U  /* Mass Storage */
#define USBD_LANGID_STRING   0x0409U  /* English */

/* ---- מחרוזות ---- */
#define MFC_STR    "STM32"
#define PROD_STR   "SPI Flash Drive"
#define CFG_STR    "MSC Config"
#define IF_STR     "MSC Interface"

/* Device Descriptor (18 bytes) */
static uint8_t DevDesc[USB_LEN_DEV_DESC] = {
    USB_LEN_DEV_DESC,          /* bLength */
    USB_DESC_TYPE_DEVICE,      /* bDescriptorType */
    0x00, 0x02,                /* bcdUSB = 2.00 */
    0x00,                      /* bDeviceClass (per-interface) */
    0x00,                      /* bDeviceSubClass */
    0x00,                      /* bDeviceProtocol */
    USB_MAX_EP0_SIZE,          /* bMaxPacketSize0 = 64 */
    LOBYTE(USBD_VID), HIBYTE(USBD_VID),
    LOBYTE(USBD_PID), HIBYTE(USBD_PID),
    0x00, 0x02,                /* bcdDevice = 2.00 */
    USBD_IDX_MFC_STR,          /* iManufacturer = 1 */
    USBD_IDX_PRODUCT_STR,      /* iProduct = 2 */
    USBD_IDX_SERIAL_STR,       /* iSerialNumber = 3 */
    USBD_MAX_NUM_CONFIGURATION /* bNumConfigurations = 1 */
};

/* Language ID string descriptor */
static uint8_t LangIDDesc[USB_LEN_LANGID_STR_DESC] = {
    USB_LEN_LANGID_STR_DESC,
    USB_DESC_TYPE_STRING,
    LOBYTE(USBD_LANGID_STRING),
    HIBYTE(USBD_LANGID_STRING),
};

/* בפר כללי למחרוזות */
static uint8_t StrDesc[USBD_MAX_STR_DESC_SIZ];

/* מספר סריאלי מ-UID של המעבד */
static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        uint8_t nibble = (value >> ((len - i - 1) * 4)) & 0x0F;
        pbuf[i * 2]     = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        pbuf[i * 2 + 1] = 0;
    }
}

/* ============================================================ */

static uint8_t *GetDeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(DevDesc);
    return DevDesc;
}

static uint8_t *GetLangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(LangIDDesc);
    return LangIDDesc;
}

static uint8_t *GetManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)MFC_STR, StrDesc, length);
    return StrDesc;
}

static uint8_t *GetProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)PROD_STR, StrDesc, length);
    return StrDesc;
}

static uint8_t *GetSerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    /*
     * Build serial from STM32 UID (96-bit = 20 displayed nibbles).
     * IntToUnicode writes 2 bytes per nibble (UTF-16LE), so:
     *   8 nibbles × 2 = 16 bytes, 4 nibbles × 2 = 8 bytes, 8 nibbles × 2 = 16 bytes
     *   Total = 40 bytes.  Buffer must be at least 40 bytes.
     */
    uint8_t serial[40];
    IntToUnicode(HAL_GetUIDw0(), serial,      8);  /* → serial[0..15]  */
    IntToUnicode(HAL_GetUIDw1(), serial + 16, 4);  /* → serial[16..23] */
    IntToUnicode(HAL_GetUIDw2(), serial + 24, 8);  /* → serial[24..39] */

    StrDesc[0] = 0;                   /* placeholder */
    StrDesc[1] = USB_DESC_TYPE_STRING;
    uint8_t idx = 2;
    /* Copy the already-Unicode bytes directly — no extra zeroes needed */
    for (int i = 0; i < 40 && idx < USBD_MAX_STR_DESC_SIZ - 1; i++) {
        StrDesc[idx++] = serial[i];
    }
    StrDesc[0] = idx;
    *length = idx;
    return StrDesc;
}

static uint8_t *GetConfigurationStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)CFG_STR, StrDesc, length);
    return StrDesc;
}

static uint8_t *GetInterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)IF_STR, StrDesc, length);
    return StrDesc;
}

/* ---- טבלת ה-callbacks ---- */
USBD_DescriptorsTypeDef FS_Desc = {
    GetDeviceDescriptor,
    GetLangIDStrDescriptor,
    GetManufacturerStrDescriptor,
    GetProductStrDescriptor,
    GetSerialStrDescriptor,
    GetConfigurationStrDescriptor,
    GetInterfaceStrDescriptor,
};
