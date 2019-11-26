/* -*- mode: asm; coding:utf-8 -*- */
/************************************************************************/
/*  OS kernel sample                                                    */
/*  Copyright 2019 Takeharu KATO                                        */
/*                                                                      */
/*  File system image                                                   */
/*                                                                      */
/************************************************************************/
#define ASM_FILE

#include <klib/asm-macros.h>
#include <hal/hal-page.h>

ASMMAC_DATA_SECTION
	
ASMMAC_BALIGN(HAL_PAGE_SIZE)
ASMMAC_DECLARE_NAME(_fsimg_start)
_fsimg_start:
	.incbin "../fsimg.img"  /* TODO: マクロから置き換える */
ASMMAC_BALIGN(HAL_PAGE_SIZE)
ASMMAC_DECLARE_NAME(_fsimg_end)
_fsimg_end:


