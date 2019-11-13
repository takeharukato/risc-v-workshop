/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V debug console                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_HAL_DBG_CONSOLE_H)
#define  _HAL_HAL_DBG_CONSOLE_H

#include <klib/freestanding.h>
#include <kern/kern-types.h>

void hal_dbg_console_init(void);
void hal_kconsole_putchar(int _ch);
#endif /*  _HAL_HAL_DBG_CONSOLE_H */
