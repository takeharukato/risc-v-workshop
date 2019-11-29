/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 Core-Local Interrupt Controller definitions              */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RV64_CLIC_H)
#define  _HAL_RV64_CLIC_H

#define CLIC_IRQ_NR          (2)  /**< 割込み総数 */
#define CLIC_IRQ_MIN         (32) /**< 最小割込み番号 */
#define CLIC_IRQ_MAX         (CLIC_IRQ_MIN + CLIC_IRQ_NR) /**< 最大割込み番号+1 */
#define CLIC_TIMER_IRQ       (32) /* タイマ割込み番号   */
#define CLIC_TIMER_PRIO      (8)  /* タイマ割込み優先度 */
#if !defined(ASM_FILE)

#include <klib/freestanding.h>

void rv64_clic_init(void);
void rv64_timer_init(void);
#endif  /* !ASM_FILE */
#endif  /* _HAL_RV64_CLIC_H  */
