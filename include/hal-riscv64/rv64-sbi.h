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
#include <kern/kern-types.h>

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

/*
 * SBI 基本拡張
 */
#define	RV64_SBI_EXT_ID_BASE			(0x10)  /**< 基本拡張IF         */
#define	RV64_SBI_BASE_GET_SPEC_VERSION	        (0)     /**< SBI版数獲得        */
#define	RV64_SBI_BASE_GET_IMPL_ID		(1)     /**< 実装ID獲得         */
#define	RV64_SBI_BASE_GET_IMPL_VERSION	        (2)     /**< 実装版数獲得       */
#define	RV64_SBI_BASE_PROBE_EXTENSION	        (3)     /**< 拡張機能の検出     */
#define	RV64_SBI_BASE_GET_MVENDORID		(4)     /**< マシンベンダID獲得 */
#define	RV64_SBI_BASE_GET_MARCHID		(5)     /**< マシンID獲得       */
#define	RV64_SBI_BASE_GET_MIMPID		(6)     /**< マシン実装ID獲得   */

#define	RV64_SBI_SPEC_VERSION_MAJOR_SHIFT       (24)    /**< SBIメジャー版数ビット位置 */
#define	RV64_SBI_SPEC_VERSION_MINOR_SHIFT       (0)     /**< SBIマイナ版数ビット位置 */

/*
 * SBI 実装ID
 */
#define	RV64_SBI_IMPL_ID_BBL		       (0)   /**< Berkeley Boot Loader (BBL)  */
#define	RV64_SBI_IMPL_ID_OPENSBI	       (1)   /**< OpenSBI */
#define	RV64_SBI_IMPL_ID_XVISOR		       (2)   /**< XVisor  */
#define	RV64_SBI_IMPL_ID_KVM 		       (3)   /**< KVM     */

/*
 * Hart State Management (HSM) 拡張
 */
#define	RV64_SBI_EXT_ID_HSM			(0x48534D)  /**< 拡張ID       */
#define	RV64_SBI_HSM_HART_START		        (0)         /**< HART起動     */
#define	RV64_SBI_HSM_HART_STOP		        (1)         /**< HART停止     */
#define	RV64_SBI_HSM_HART_STATUS		(2)         /**< HART状態確認 */
#define	RV64_SBI_HSM_STATUS_STARTED		(0)         /**< HART起動中   */
#define	RV64_SBI_HSM_STATUS_STOPPED		(1)         /**< HART停止中   */
#define	RV64_SBI_HSM_STATUS_START_PENDING	(2)         /**< HART起動待ち */
#define	RV64_SBI_HSM_STATUS_STOP_PENDING	(3)         /**< HART停止待ち */

/**
   SBIメジャー版数マスク
*/
#define	RV64_SBI_SPEC_VERSION_MAJOR_MASK        \
	( ( ULONGLONG_C(1) << (7) ) - 1)
/**
   SBIマイナ版数マスク
*/
#define	RV64_SBI_SPEC_VERSION_MINOR_MASK        \
	( ( ULONGLONG_C(1) << RV64_SBI_SPEC_VERSION_MAJOR_SHIFT ) - 1)

/**
   引数を4つとるSBI呼び出し
   @param[in] _ext  SBI callの拡張番号 (Extension ID)
   @param[in] _func SBI callの機能番号 (Function ID)
   @param[in] _arg0 SBI callの第1引数
   @param[in] _arg1 SBI callの第2引数
   @param[in] _arg2 SBI callの第3引数
   @param[in] _arg3 SBI callの第4引数
   @return SBI返却値 (sbi_ret構造体)
 */
#define RV64_SBICALL4(_ext, _func, _arg0, _arg1, _arg2, _arg3)		\
	rv64_sbi_call((_ext), (_func), (_arg0), (_arg1), (_arg2), (_arg3))

/**
   引数を3つとるSBI呼び出し
   @param[in] _ext  SBI callの拡張番号 (Extension ID)
   @param[in] _func SBI callの機能番号 (Function ID)
   @param[in] _arg0 SBI callの第1引数
   @param[in] _arg1 SBI callの第2引数
   @param[in] _arg2 SBI callの第3引数
   @return SBI返却値 (sbi_ret構造体)
 */
#define RV64_SBICALL3(_ext, _func, _arg0, _arg1, _arg2)			\
	rv64_sbi_call((_ext), (_func), (_arg0), (_arg1), (_arg2), (0))

/**
   引数を2つとるSBI呼び出し
   @param[in] _ext  SBI callの拡張番号 (Extension ID)
   @param[in] _func SBI callの機能番号 (Function ID)
   @param[in] _arg0 SBI callの第1引数
   @param[in] _arg1 SBI callの第2引数
   @return SBI返却値 (sbi_ret構造体)
 */
#define RV64_SBICALL2(_ext, _func, _arg0, _arg1)			\
	rv64_sbi_call((_ext), (_func), (_arg0), (_arg1), (0), (0))

/**
   引数を1つとるSBI呼び出し
   @param[in] _ext  SBI callの拡張番号 (Extension ID)
   @param[in] _func SBI callの機能番号 (Function ID)
   @param[in] _arg0 SBI callの第1引数
   @return SBI返却値 (sbi_ret構造体)
 */
#define RV64_SBICALL1(_ext, _func, _arg0)			\
	rv64_sbi_call((_ext), (_func), (_arg0), (0), (0), (0))

/**
   引数をとらないSBI呼び出し
   @param[in] _ext  SBI callの拡張番号 (Extension ID)
   @param[in] _func SBI callの機能番号 (Function ID)
   @return SBI返却値 (sbi_ret構造体)
 */
#define RV64_SBICALL0(_ext, _func)				\
	rv64_sbi_call((_ext), (_func), (0), (0), (0), (0))

/**
   SBI返却値
 */
typedef struct _rv64_sbi_sbiret {
        long error;  /**< エラーコード */
        long value;  /**< 返却値       */
}rv64_sbi_sbiret;

struct _rv64_sbi_sbiret rv64_sbi_call(uint64_t _func, uint64_t _ext, uint64_t _arg0, uint64_t _arg1, uint64_t _arg2, uint64_t _arg3);

struct _rv64_sbi_sbiret rv64_sbi_get_spec_version(uint32_t *_majorp, uint32_t *_minorp);
struct _rv64_sbi_sbiret rv64_sbi_get_impl_id(uint64_t *_implp);
struct _rv64_sbi_sbiret rv64_sbi_probe_extension(int64_t _id);
void rv64_sbi_set_timer(reg_type next_time_val);
void rv64_sbi_shutdown(void);
void rv64_sbi_clear_ipi(void);
void rv64_sbi_send_ipi(const unsigned long *_hart_mask);
void rv64_sbi_remote_fence_i(const unsigned long *_hart_mask);
void rv64_sbi_remote_sfence_vma(const unsigned long *_hart_mask,
    vm_vaddr _start, vm_size _size);
void rv64_sbi_remote_sfence_vma_asid(const unsigned long *_hart_mask,
    vm_vaddr _start, vm_size _size, uint64_t _asid);

struct _rv64_sbi_sbiret rv64_sbi_hsm_hart_start(uint64_t _hart, vm_paddr _start_addr, uint64_t _private);
void rv64_sbi_hsm_hart_stop(void);
int rv64_sbi_hsm_hart_status(uint64_t _hart, int *_statusp);

#endif  /* !ASM_FILE */
#endif  /* _HAL_RV64_SBI_H  */
