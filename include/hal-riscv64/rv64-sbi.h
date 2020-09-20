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
#include <kern/kern-cpuinfo.h>

#define RV64_SBI_SUCCESS                    (0)   /**< 正常終了                 */
#define RV64_SBI_ERR_FAILURE                (-1)  /**< 失敗                     */
#define RV64_SBI_ERR_NOT_SUPPORTED          (-2)  /**< 未サポート               */
#define RV64_SBI_ERR_INVALID_PARAM          (-3)  /**< 引数異常                 */
#define RV64_SBI_ERR_DENIED                 (-4)  /**< リクエストを拒否された   */
#define RV64_SBI_ERR_INVALID_ADDRESS        (-5)  /**< 不正なアドレスを指定した */

#define	RV64_SBI_SET_TIMER			(0)  /**< タイマの設定                     */
#define	RV64_SBI_CONSOLE_PUTCHAR		(1)  /**< コンソールへの出力               */
#define	RV64_SBI_CONSOLE_GETCHAR		(2)  /**< コンソールからの入力             */
#define	RV64_SBI_CLEAR_IPI			(3)  /**< IPI受付完了                      */
#define	RV64_SBI_SEND_IPI			(4)  /**< IPI送信                          */
#define	RV64_SBI_REMOTE_FENCE_I		(5)  /**< 命令完了待ち合せ                 */
#define	RV64_SBI_REMOTE_SFENCE_VMA		(6)  /**< メモリストア完了待ち合せ         */
#define	RV64_SBI_REMOTE_SFENCE_VMA_ASID	(7)  /**< ASID付きメモリストア完了待ち合せ */
#define	RV64_SBI_SHUTDOWN			(8)  /**< シャットダウン                   */

/* SBI Base Extension */
#define	RV64_SBI_EXT_ID_BASE			(0x10)
#define	RV64_SBI_BASE_GET_SPEC_VERSION	        (0)
#define	RV64_SBI_BASE_GET_IMPL_ID		(1)
#define	RV64_SBI_BASE_GET_IMPL_VERSION	        (2)
#define	RV64_SBI_BASE_PROBE_EXTENSION	        (3)
#define	RV64_SBI_BASE_GET_MVENDORID		(4)
#define	RV64_SBI_BASE_GET_MARCHID		(5)
#define	RV64_SBI_BASE_GET_MIMPID		(6)

#define	RV64_SBI_SPEC_VERSION_MAJOR_SHIFT       (24)
#define	RV64_SBI_SPEC_VERSION_MINOR_SHIFT       (0)
#define	RV64_SBI_SPEC_VERSION_MAJOR_MASK        \
	( ( ULONGLONG_C(1) << (7) ) - 1)
#define	RV64_SBI_SPEC_VERSION_MINOR_MASK        \
	( ( ULONGLONG_C(1) << RV64_SBI_SPEC_VERSION_MAJOR_SHIFT ) - 1)
/**
   SBIエラーコード
 */
typedef struct _rv64_sbi_sbiret {
        long error;  /**< エラーコード */
        long value;  /**< 返却値       */
}rv64_sbi_sbiret;

struct _rv64_sbi_sbiret rv64_sbi_call(uint64_t _func, uint64_t _ext, uint64_t _arg0, uint64_t _arg1, uint64_t _arg2, uint64_t _arg3);

struct _rv64_sbi_sbiret rv64_sbi_get_spec_version(uint32_t *_majorp, uint32_t *_minorp);
#endif  /* !ASM_FILE */
#endif  /* _HAL_RV64_SBI_H  */
