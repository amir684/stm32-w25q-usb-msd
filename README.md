# STM32F401 USB Mass Storage — W25Q64 SPI Flash Drive

Bare-metal PlatformIO project that exposes an **8 MB W25Q64 SPI NOR Flash chip** as a standard USB Mass Storage Device (MSC/BOT/SCSI) on a **WeAct Black Pill** (STM32F401RC).

Plug the board into any PC and it appears as an 8 MB USB flash drive — no drivers needed.

---

## Hardware

| Signal | MCU pin | Notes |
|--------|---------|-------|
| USB D− | PA11 | USB OTG FS |
| USB D+ | PA12 | USB OTG FS |
| SPI SCK | PA5 | SPI1 |
| SPI MISO | PA6 | SPI1 |
| SPI MOSI | PA7 | SPI1 |
| Flash CS | PA4 | GPIO, active-low |
| LED | PC13 | Active-low (Black Pill built-in) |
| HSE crystal | — | 25 MHz (required for USB ±0.25 %) |

**Flash chip:** Winbond W25Q64 (8 MB, 4 KB sectors, 256 B pages)

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                        main.c                           │
│  HAL_Init → Clock → GPIO → SPI → W25Q_Init → USB_Init  │
│  while(1): LED heartbeat (3 blinks + 800 ms pause)      │
└────────────────────────┬────────────────────────────────┘
                         │
          ┌──────────────▼──────────────┐
          │       USB OTG FS core       │
          │   usbd_conf.c / usbd_desc.c │
          │   PCD HAL ↔ USBD middleware │
          └──────────────┬──────────────┘
                         │  SCSI/BOT callbacks
          ┌──────────────▼──────────────┐
          │    usbd_storage_if.c        │
          │  Read / Write / GetCapacity │
          └──────────────┬──────────────┘
                         │
          ┌──────────────▼──────────────┐
          │          w25q.c             │
          │   SPI1 polling driver       │
          │  ReadData / WritePage /     │
          │  EraseSector / ReadID       │
          └─────────────────────────────┘
```

### Key source files

| File | Purpose |
|------|---------|
| `src/main.c` | Entry point — clock, peripherals, USB init, LED heartbeat |
| `src/usbd_conf.c` | USB HAL glue: PCD init, FIFO sizing, IRQ, static allocator |
| `src/usbd_desc.c` | USB descriptors (Device, Configuration, String) |
| `src/usb_device.c` | `MX_USB_DEVICE_Init()` — registers MSC class + storage callbacks |
| `src/usbd_storage_if.c` | SCSI storage backend: maps 512 B blocks to W25Q64 sectors |
| `src/w25q.c` | Low-level W25Q SPI driver (read / page-program / sector-erase) |
| `src/stm32f4xx_it.c` | SysTick_Handler + HardFault_Handler |
| `src/stm32f4xx_hal_msp.c` | SPI1 GPIO alternate-function init |
| `include/usbd_conf.h` | USBD middleware configuration macros |
| `include/stm32f4xx_hal_conf.h` | HAL module selection + SysTick priority |
| `scripts/usb_middleware.py` | PlatformIO pre-script: adds ST USB middleware to the build |

---

## Clock configuration

USB Full Speed requires the 48 MHz clock to be within **±0.25 %** — only an HSE crystal meets this tolerance.

```
HSE 25 MHz  →  PLL  →  SYSCLK 84 MHz,  USB 48.000 MHz (exact)

  VCO  = 25 / PLLM(25) × PLLN(336) = 336 MHz
  SYSCLK = VCO / PLLP(4)  = 84 MHz
  USB    = VCO / PLLQ(7)  = 48 MHz  ← exact, ±0 ppm from PLL
```

---

## USB stack

The project uses the **ST USB Device Library** (shipped inside `framework-stm32cubef4`) without any modification. A PlatformIO pre-script (`scripts/usb_middleware.py`) adds the Core and MSC source files to the build automatically.

### Static memory allocator

The USB MSC middleware calls `USBD_malloc` once to allocate `USBD_MSC_BOT_HandleTypeDef` (~628 bytes). Newlib-nano's default heap is only 512 bytes — not enough.

Solution: a simple 768-byte static pool defined in `usbd_conf.c`. No heap dependency, no fragmentation.

```c
#define USBD_MEM_SIZE 768U
static uint8_t usbd_mem_pool[USBD_MEM_SIZE];

void *USBD_static_malloc(uint32_t size) {
    if (size <= USBD_MEM_SIZE) {
        memset(usbd_mem_pool, 0, size);
        return usbd_mem_pool;
    }
    return NULL;
}
```

### USB descriptors

| Field | Value |
|-------|-------|
| VID | `0x0483` (STMicroelectronics) |
| PID | `0x5720` (Mass Storage) |
| Manufacturer | `STM32` |
| Product | `SPI Flash Drive` |
| Serial | Derived from STM32 UID (96-bit), UTF-16LE encoded |
| bcdUSB | 2.00 |
| Max packet (FS) | 64 bytes |

### FIFO layout (USB OTG FS, 1280 bytes total)

| FIFO | Size | Purpose |
|------|------|---------|
| RxFIFO | 512 B (0x80 words) | All OUT/SETUP traffic |
| TxFIFO EP0 | 256 B (0x40 words) | Control IN |
| TxFIFO EP1 | 512 B (0x80 words) | Bulk IN (MSC data) |

---

## Storage layer

The W25Q64 has a **4 KB erase granularity** but USB/SCSI works in **512 B blocks**. The storage interface bridges this with a read-modify-write strategy:

```
Write(blk_addr, blk_len):
  for each 4 KB sector touched:
    1. Read full 4 KB sector → temp buffer
    2. Patch the 512 B blocks that changed
    3. Erase sector (4 KB)
    4. Write back 16 × 256 B pages
```

A 4 KB static buffer (`_sector_buf`) holds the sector during the operation.

**Flash geometry:**
| Parameter | Value |
|-----------|-------|
| Capacity | 8 MB (8,388,608 bytes) |
| SCSI blocks | 16,384 × 512 B |
| Sector size | 4,096 B (erase unit) |
| Page size | 256 B (write unit) |
| SPI clock | 21 MHz (PCLK2 84 MHz / 4) |

---

## LED status codes

| Pattern | Meaning |
|---------|---------|
| Solid ON at boot | Init in progress |
| 3 short blinks, 800 ms pause (repeat) | Running normally, waiting for USB host |
| Rapid toggling (~10 Hz) | HardFault — firmware crashed |
| Fast blink in `Error_Handler` | HAL peripheral init failed |

---

## Building & flashing

### Requirements
- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- `dfu-util` (installed automatically by PlatformIO)
- STM32F401RC Black Pill board with W25Q64 wired as shown above

### Build
```bash
pio run
```

### Flash (DFU bootloader)
1. Hold **BOOT0** on the Black Pill
2. Tap **RESET**
3. Release **BOOT0**
4. Run:
```bash
pio run -t upload
```

---

## Known design notes & gotchas

### SysTick priority must be 0 (highest)
`TICK_INT_PRIORITY = 0x00U` in `stm32f4xx_hal_conf.h`. The USB ISR runs at priority 5. SysTick must be able to preempt it, otherwise `HAL_Delay()` deadlocks inside USB callbacks.

### `SysTick_Handler` must be in your project
STM32Cube projects require `SysTick_Handler` calling `HAL_IncTick()` in `stm32f4xx_it.c`. Without it, `uwTick` never increments and every `HAL_Delay()` hangs forever.

### Serial string descriptor buffer size
`IntToUnicode()` writes **2 bytes per nibble** (UTF-16LE). The UID serial uses 20 nibbles = 40 bytes. The local buffer must be `uint8_t serial[40]`, not 24 — a 24-byte buffer overflows the stack and causes a HardFault during USB enumeration (when Windows requests string descriptor #3).

### HSI cannot be used for USB
The internal RC oscillator (HSI 16 MHz) has ±1 % tolerance — 4× worse than USB FS requires. Always use the HSE crystal for the USB clock source.

---

## License

MIT — do whatever you like, no warranty expressed or implied.
