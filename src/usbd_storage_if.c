/**
 * usbd_storage_if.c – bridge between USB MSC and W25Q flash
 *
 * WHY A WRITE-BACK CACHE EXISTS HERE
 * ────────────────────────────────────────────────────────────────────────────
 * The USB/SCSI layer (SCSI_ProcessWrite) always calls STORAGE_Write_FS with
 * exactly one 512 B block at a time.  NOR flash erase granularity is 4 KB
 * (8 blocks).  Without a cache, every 512 B write triggers:
 *
 *   Read  4 KB from flash      ~0.2 ms   (SPI @ 21 MHz)
 *   Erase 4 KB sector          ~45 ms    (W25Q64 typical)
 *   Write 16 × 256 B pages     ~48 ms    (3 ms/page)
 *   ─────────────────────────────────────
 *   Total per 512 B block      ~93 ms
 *
 * For a sequential 3 MB file: 6144 calls × 93 ms ≈ 9 minutes.
 * Each of the 8 blocks in the same 4 KB sector erases it redundantly.
 *
 * WITH CACHE (this implementation)
 * ──────────────────────────────────
 * Incoming 512 B blocks are patched into a 4 KB RAM buffer.
 * The sector is only erased+written when:
 *   • the next write is to a *different* sector  (flush old, load new), or
 *   • a read request arrives                     (flush first, read flash).
 *
 * Result: 1 erase per 4 KB instead of 8  →  ~8× faster sequential writes.
 * For 3 MB: ~768 sector ops × 93 ms ≈ 71 s  →  still slow but usable.
 *
 * THREAD SAFETY
 * ─────────────
 * All five STORAGE_*_FS functions are called exclusively from the USB OTG FS
 * interrupt (priority 5).  There is no concurrent access — no locks needed.
 */

#include "usbd_storage_if.h"
#include "w25q.h"
#include <string.h>

/* ---- geometry ---- */
#define BLOCK_SIZE        512U
#define SECTOR_SIZE       W25Q_SECTOR_SIZE           /* 4096 B */
#define BLOCKS_PER_SECTOR (SECTOR_SIZE / BLOCK_SIZE) /* 8      */
#define TOTAL_BLOCKS      (W25Q_CAPACITY_BYTES / BLOCK_SIZE)

/* ---- SCSI Inquiry data (36 bytes, fixed layout) ---- */
int8_t STORAGE_Inquirydata_FS[] = {
    0x00,       /* Peripheral device type = Direct Access */
    0x80,       /* Removable bit */
    0x02,       /* Version = SPC-2 */
    0x02,       /* Response data format */
    0x1F,       /* Additional length (31 = 36-5) */
    0x00, 0x00, 0x00,
    'S','T','M','3','2',' ',' ',' ',            /* Vendor  (8 B) */
    'S','P','I',' ','F','l','a','s','h',' ','D','r','i','v','e',' ', /* Product (16 B) */
    '1','.','0','0'                              /* Revision (4 B) */
};

/* ════════════════════════════════════════════════════════════════════════════
   Write-back cache
   ════════════════════════════════════════════════════════════════════════════ */

/* One 4 KB buffer holds the sector currently being accumulated. */
static uint8_t  _cache_buf[SECTOR_SIZE];

/*
 * Address of the sector whose data lives in _cache_buf.
 * 0xFFFFFFFF means "cache is empty / clean".
 */
#define CACHE_EMPTY 0xFFFFFFFFUL
static uint32_t _cached_sector = CACHE_EMPTY;

/*
 * flush_cache() – commit the cached sector to flash.
 * Erase the sector, then write back all 16 pages.
 * Clears _cached_sector when done.
 */
static void flush_cache(void)
{
    if (_cached_sector == CACHE_EMPTY) return;

    W25Q_EraseSector(_cached_sector);
    for (uint32_t p = 0; p < SECTOR_SIZE; p += W25Q_PAGE_SIZE)
        W25Q_WritePage(_cached_sector + p, _cache_buf + p, W25Q_PAGE_SIZE);

    _cached_sector = CACHE_EMPTY;
}

/* ════════════════════════════════════════════════════════════════════════════
   STORAGE callbacks
   ════════════════════════════════════════════════════════════════════════════ */

static int8_t STORAGE_Init_FS(uint8_t lun)
{
    (void)lun;
    /*
     * Called when the USB host initialises the MSC class (including after a
     * USB reset / reconnect).  Flush any dirty sector so a reconnect always
     * starts with a clean flash state.
     */
    flush_cache();
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
    return 0;
}

static int8_t STORAGE_IsWriteProtected_FS(uint8_t lun)
{
    (void)lun;
    return 0;
}

/*
 * STORAGE_Read_FS – read blk_len × 512 B from flash.
 *
 * Must flush the cache first: if the host reads back what it just wrote,
 * the data is still in _cache_buf and hasn't reached flash yet.
 */
static int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf,
                               uint32_t blk_addr, uint16_t blk_len)
{
    (void)lun;
    flush_cache();
    W25Q_ReadData((uint32_t)blk_addr * BLOCK_SIZE, buf, (uint32_t)blk_len * BLOCK_SIZE);
    return 0;
}

/*
 * STORAGE_Write_FS – write blk_len × 512 B blocks (always called with blk_len=1
 * by the SCSI layer, but the loop handles any value correctly).
 *
 * Strategy:
 *   1. Compute which 4 KB sector the block belongs to.
 *   2. If a different sector is cached → flush it, then load the new sector.
 *   3. If no sector is cached         → load the target sector from flash.
 *   4. Patch the 512 B slice in _cache_buf.
 *   5. Return — do NOT write to flash yet.  The flash write is deferred
 *      until flush_cache() is called (on next sector change or on read).
 */
static int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf,
                                uint32_t blk_addr, uint16_t blk_len)
{
    (void)lun;

    for (uint16_t i = 0; i < blk_len; i++)
    {
        uint32_t byte_addr   = ((uint32_t)blk_addr + i) * BLOCK_SIZE;
        uint32_t sector_addr = (byte_addr / SECTOR_SIZE) * SECTOR_SIZE;
        uint32_t blk_offset  = (byte_addr - sector_addr) / BLOCK_SIZE;

        if (sector_addr != _cached_sector)
        {
            /* Moving to a new sector: commit old one, load new one. */
            flush_cache();
            W25Q_ReadData(sector_addr, _cache_buf, SECTOR_SIZE);
            _cached_sector = sector_addr;
        }

        /* Patch this 512 B slice into the cached sector image. */
        memcpy(_cache_buf + blk_offset * BLOCK_SIZE,
               buf + (uint32_t)i * BLOCK_SIZE,
               BLOCK_SIZE);
    }

    return 0; /* flash write deferred */
}

static int8_t STORAGE_GetMaxLun_FS(void)
{
    return 0; /* single LUN */
}

/* ---- callback table ---- */
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
