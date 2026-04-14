/**
 * usbd_storage_if.c – הגשר בין USB MSC לבין W25Q32
 *
 * זה הקובץ שעושה את הקסם:
 *   מחשב שולח/קורא בלוקים של 512 bytes (SCSI)
 *   W25Q עובד בעמודים של 256B וסקטורים של 4096B
 *
 * כתיבה = read-modify-write ברמת סקטור 4KB
 */
#include "usbd_storage_if.h"
#include "w25q.h"
#include <string.h>

/* ---- גדלים ---- */
#define BLOCK_SIZE        512U                          /* SCSI block */
#define SECTOR_SIZE       W25Q64_SECTOR_SIZE            /* 4096B */
#define BLOCKS_PER_SECTOR (SECTOR_SIZE / BLOCK_SIZE)   /* 8 */
#define TOTAL_BLOCKS      (W25Q64_CAPACITY_BYTES / BLOCK_SIZE) /* 16384 */

/* ---- SCSI Inquiry data (36 bytes) ---- */
int8_t STORAGE_Inquirydata_FS[] = {
    /* Byte 0: Peripheral device type = Direct Access */
    0x00,
    /* Byte 1: Removable bit */
    0x80,
    /* Byte 2: Version = SPC-2 */
    0x02,
    /* Byte 3: Response data format */
    0x02,
    /* Byte 4: Additional length (31 = 36-5) */
    0x1F,
    /* Bytes 5-7: Reserved */
    0x00, 0x00, 0x00,
    /* Bytes 8-15: Vendor Identification (8 bytes) */
    'S','T','M','3','2',' ',' ',' ',
    /* Bytes 16-31: Product Identification (16 bytes) */
    'S','P','I',' ','F','l','a','s','h',' ','D','r','i','v','e',' ',
    /* Bytes 32-35: Product Revision (4 bytes) */
    '1','.','0','0'
};

/* ---- פונקציות ---- */

static int8_t STORAGE_Init_FS(uint8_t lun)
{
    (void)lun;
    /* W25Q_Init כבר נקרא ב-main.c לפני USB init */
    return 0;
}

static int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
    (void)lun;
    *block_num  = TOTAL_BLOCKS;
    *block_size = BLOCK_SIZE;
    return 0;
}

static int8_t STORAGE_IsReady_FS(uint8_t lun)
{
    (void)lun;
    return 0; /* 0 = מוכן */
}

static int8_t STORAGE_IsWriteProtected_FS(uint8_t lun)
{
    (void)lun;
    return 0; /* 0 = לא מוגן */
}

/* ---- קריאה: ישירה ופשוטה ---- */
static int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf,
                                uint32_t blk_addr, uint16_t blk_len)
{
    (void)lun;
    W25Q_ReadData(blk_addr * BLOCK_SIZE, buf, (uint32_t)blk_len * BLOCK_SIZE);
    return 0;
}

/* ---- כתיבה: read-modify-write ברמת סקטור 4KB ---- */
static uint8_t _sector_buf[SECTOR_SIZE];  /* בפר עזר = 4KB */

static int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf,
                                 uint32_t blk_addr, uint16_t blk_len)
{
    (void)lun;

    uint16_t i = 0;
    while (i < blk_len)
    {
        uint32_t byte_addr    = (blk_addr + i) * BLOCK_SIZE;
        uint32_t sector_addr  = (byte_addr / SECTOR_SIZE) * SECTOR_SIZE;

        /* כמה בלוקים נכתוב בסקטור הנוכחי */
        uint32_t offset_blk   = (byte_addr - sector_addr) / BLOCK_SIZE;
        uint16_t blk_in_sector = (uint16_t)(BLOCKS_PER_SECTOR - offset_blk);
        if (blk_in_sector > (blk_len - i))
            blk_in_sector = (uint16_t)(blk_len - i);

        /* 1. קרא את הסקטור הנוכחי */
        W25Q_ReadData(sector_addr, _sector_buf, SECTOR_SIZE);

        /* 2. עדכן את החלק שצריך */
        memcpy(_sector_buf + offset_blk * BLOCK_SIZE,
               buf + i * BLOCK_SIZE,
               blk_in_sector * BLOCK_SIZE);

        /* 3. מחק סקטור */
        W25Q_EraseSector(sector_addr);

        /* 4. כתוב בחזרה עמוד-עמוד (256B) */
        for (uint32_t p = 0; p < SECTOR_SIZE; p += W25Q64_PAGE_SIZE)
        {
            W25Q_WritePage(sector_addr + p, _sector_buf + p, W25Q64_PAGE_SIZE);
        }

        i += blk_in_sector;
    }

    return 0;
}

static int8_t STORAGE_GetMaxLun_FS(void)
{
    return 0; /* LUN אחד */
}

/* ---- טבלת קריאות-חזרה לUSBD ---- */
USBD_StorageTypeDef USBD_Storage_Interface_fops_FS = {
    STORAGE_Init_FS,
    STORAGE_GetCapacity_FS,
    STORAGE_IsReady_FS,
    STORAGE_IsWriteProtected_FS,
    STORAGE_Read_FS,
    STORAGE_Write_FS,
    STORAGE_GetMaxLun_FS,
    STORAGE_Inquirydata_FS,
};
