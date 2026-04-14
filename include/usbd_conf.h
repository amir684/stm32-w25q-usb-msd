#pragma once

#include <string.h>
#include "stm32f4xx_hal.h"

/* ---- הגדרות USB Device ---- */
#define USBD_MAX_NUM_INTERFACES       1U
#define USBD_MAX_NUM_CONFIGURATION    1U
#define USBD_MAX_STR_DESC_SIZ         0x100U
#define USBD_SELF_POWERED             1U
#define USBD_DEBUG_LEVEL              0U

/* ---- MSC ---- */
#define MSC_MEDIA_PACKET              512U

/* ---- ניהול זיכרון — allocator סטטי, ללא תלות ב-heap ---- */
void *USBD_static_malloc(uint32_t size);
void  USBD_static_free(void *p);
#define USBD_malloc    USBD_static_malloc
#define USBD_free      USBD_static_free
#define USBD_memset    memset
#define USBD_memcpy    memcpy

/* ---- Debug (מושתק) ---- */
#define USBD_UsrLog(...)
#define USBD_ErrLog(...)
#define USBD_DbgLog(...)
