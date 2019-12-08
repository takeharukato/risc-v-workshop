/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 Platform-Level Interrupt Controller definitions          */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RV64_PLIC_H)
#define  _HAL_RV64_PLIC_H
#include <klib/misc.h>
#include <klib/klib-consts.h>

#define PLIC_IRQ_NR          (32)  /**< 割込み総数 */
#define PLIC_IRQ_MIN         (0)   /**< 最小割込み番号 */
#define PLIC_IRQ_MAX         (PLIC_IRQ_MIN + PLIC_IRQ_NR) /**< 最大割込み番号+1 */
#define PLIC_PRIO_DIS        (0)   /**< 割込み無効時の割込み優先度 */
#define PLIC_PRIO_THRES_ALL  (0)   /**< 割込みマスク無効時の割込み優先度 */
#define PLIC_PRIO_MIN        (1)   /**< 最小割込み優先度 */
#define PLIC_PRIO_MAX        (7)   /**< 最大割込み優先度 */

/**< 割込みコントローラレジスタ長(単位:バイト) */
#define PLIC_REGSIZE         (ULONGLONG_C(4))
/**< 優先度設定レジスタオフセット(単位:バイト) */
#define PLIC_PRIO_OFFSET     (ULONGLONG_C(0x0)) 
 /**< 保留割込みレジスタオフセット(単位:バイト) */
#define PLIC_PEND_OFFSET     (ULONGLONG_C(0x1000))
/**< マシンモード割込み許可レジスタオフセット(単位:バイト) */
#define PLIC_MENABLE_OFFSET  (ULONGLONG_C(0x2000))
/**< スーパーバイザモード割込み許可レジスタオフセット(単位:バイト) */
#define PLIC_SENABLE_OFFSET  (ULONGLONG_C(0x2080))
/**< hart単位での割込み許可レジスタ長(単位:バイト) */
#define PLIC_ENABLE_PER_HART (ULONGLONG_C(0x100))
/**< マシンモード割込み優先度レジスタオフセット(単位:バイト) */
#define PLIC_MPRIO_OFFSET    (ULONGLONG_C(0x200000))
/**< スーパーバイザモード割込み優先度レジスタオフセット(単位:バイト) */
#define PLIC_SPRIO_OFFSET    (ULONGLONG_C(0x201000))
/**< hart単位での割込み優先度レジスタ長(単位:バイト) */
#define PLIC_PRIO_PER_HART   (ULONGLONG_C(0x2000))
/**< マシンモード割込みクレームレジスタオフセット(単位:バイト) */
#define PLIC_MCLAIM_OFFSET   (ULONGLONG_C(0x200004))
/**< スーパーバイザモード割込みクレームレジスタオフセット(単位:バイト) */
#define PLIC_SCLAIM_OFFSET   (ULONGLONG_C(0x201004))
/**< hart単位での割込みクレームレジスタ長(単位:バイト) */
#define PLIC_CLAIM_PER_HART  (ULONGLONG_C(0x2000))

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
typedef uint32_t                plic_reg;  /* PLICのレジスタ     */
typedef volatile uint32_t * plic_reg_ref;  /* PLICのレジスタ参照 */

/**
   PLIC優先度レジスタ取得
   @param[in] _irq 割込み番号
   @return PLIC優先度レジスタアドレス
 */
#define PLIC_PRIO_REG(_irq) \
	((plic_reg_ref)(RV64_PLIC + PLIC_PRIO_OFFSET + ( (_irq) * PLIC_REGSIZE ) ))

/**
   PLIC割込みペンディングレジスタ取得
   @param[in] _irq 割込み番号
   @return PLIC割込みペンディングレジスタアドレス
 */
#define PLIC_PEND_REG(_irq) \
	((plic_reg_ref)(RV64_PLIC + PLIC_PEND_OFFSET \
	    + ( (_irq) / ( PLIC_REGSIZE * BITS_PER_BYTE ) ) ) )

/**
   マシンモード割込み許可レジスタ取得
   @param[in] _hart 物理プロセッサID (hart番号)
   @return マシンモード割込み許可レジスタアドレス
 */
#define PLIC_MENABLE_REG(_hart) \
	((plic_reg_ref)(RV64_PLIC + PLIC_MENABLE_OFFSET + ( (_hart) * PLIC_ENABLE_PER_HART) ) )

/**
   スーパーバイザモード割込み許可レジスタ取得
   @param[in] _hart 物理プロセッサID (hart番号)
   @return スーパーバイザモード割込み許可レジスタアドレス
 */
#define PLIC_SENABLE_REG(_hart) \
	((plic_reg_ref)(RV64_PLIC + PLIC_SENABLE_OFFSET + ( (_hart) * PLIC_ENABLE_PER_HART) ) )

/**
   マシンモード割込み優先度レジスタ取得
   @param[in] _hart 物理プロセッサID (hart番号)
   @return マシンモード割込み優先度レジスタアドレス
 */
#define PLIC_MPRIO_REG(_hart) \
	((plic_reg_ref)(RV64_PLIC + PLIC_MPRIO_OFFSET + ( (_hart) * PLIC_PRIO_PER_HART) ) )

/**
   スーパーバイザモード割込み優先度レジスタ取得
   @param[in] _hart 物理プロセッサID (hart番号)
   @return スーパーバイザモード割込み優先度レジスタアドレス
 */
#define PLIC_SPRIO_REG(_hart) \
	((plic_reg_ref)(RV64_PLIC + PLIC_SPRIO_OFFSET + ( (_hart) * PLIC_PRIO_PER_HART) ) )

/**
   マシンモード割込みクレームレジスタ取得
   @param[in] _hart 物理プロセッサID (hart番号)
   @return マシンモード割込みクレームレジスタアドレス
 */
#define PLIC_MCLAIM_REG(_hart) \
	((plic_reg_ref)(RV64_PLIC + PLIC_MCLAIM_OFFSET + ( (_hart) * PLIC_CLAIM_PER_HART) ) )

/**
   スーパーバイザモード割込みクレームレジスタ取得
   @param[in] _hart 物理プロセッサID (hart番号)
   @return スーパーバイザモード割込みクレームレジスタアドレス
 */
#define PLIC_SCLAIM_REG(_hart) \
	((plic_reg_ref)(RV64_PLIC + PLIC_SCLAIM_OFFSET + ( (_hart) * PLIC_CLAIM_PER_HART) ) )

irq_prio rv64_plic_set_priority_mask(irq_prio _prio);
void rv64_plic_init(void);
#endif  /* !ASM_FILE */
#endif  /* _HAL_RV64_PLIC_H  */
