/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 Core Local Interruptor definitions                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RV64_CLINT_H)
#define  _HAL_RV64_CLINT_H

#define CLINT_IRQ_NR          (2)  /**< 割込み総数 */
#define CLINT_IRQ_MIN         (32) /**< 最小割込み番号 */
#define CLINT_IRQ_MAX         (CLINT_IRQ_MIN + CLINT_IRQ_NR) /**< 最大割込み番号+1 */
#define CLINT_TIMER_IRQ       (32) /**< タイマ割込み番号   */
#define CLINT_TIMER_PRIO      (8)  /**< タイマ割込み優先度 */
#define CLINT_IPI_IRQ         (33) /**< プロセッサ間割込み番号   */
#define CLINT_IPI_PRIO        (8)  /**< プロセッサ間割込み優先度 */

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/kern-types.h>

void rv64_clint_init(void);
void rv64_timer_init(void);
void rv64_ipi_init(void);
#endif  /* !ASM_FILE */
#endif  /* _HAL_RV64_CLINT_H  */
