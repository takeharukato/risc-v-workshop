/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 Supervisor Binary Interface definitions                 */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RV64_SBI_H)
#define  _HAL_RV64_SBI_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>

#define SBI_SUCCESS                    (0)   /**< 正常終了                 */
#define SBI_ERR_FAILURE                (-1)  /**< 失敗                     */
#define SBI_ERR_NOT_SUPPORTED          (-2)  /**< 未サポート               */
#define SBI_ERR_INVALID_PARAM          (-3)  /**< 引数異常                 */
#define SBI_ERR_DENIED                 (-4)  /**< リクエストを拒否された   */
#define SBI_ERR_INVALID_ADDRESS        (-5)  /**< 不正なアドレスを指定した */

#define	SBI_SET_TIMER			(0)  /**< タイマの設定                     */
#define	SBI_CONSOLE_PUTCHAR		(1)  /**< コンソールへの出力               */
#define	SBI_CONSOLE_GETCHAR		(2)  /**< コンソールからの入力             */
#define	SBI_CLEAR_IPI			(3)  /**< IPI受付完了                      */
#define	SBI_SEND_IPI			(4)  /**< IPI送信                          */
#define	SBI_REMOTE_FENCE_I		(5)  /**< 命令完了待ち合せ                 */
#define	SBI_REMOTE_SFENCE_VMA		(6)  /**< メモリストア完了待ち合せ         */
#define	SBI_REMOTE_SFENCE_VMA_ASID	(7)  /**< ASID付きメモリストア完了待ち合せ */
#define	SBI_SHUTDOWN			(8)  /**< シャットダウン                   */

/**
   エラーコード
 */
struct sbiret {
        long error;  /**< エラーコード */
        long value;  /**< 返却値       */
};

void ksbi_send_ipi(const unsigned long *_hart_mask);
#endif  /* !ASM_FILE */
#endif  /* _HAL_RV64_SBI_H  */
