/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 machine mode routines                                    */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1
#include <kern/kern-consts.h>
#include <klib/asm-macros.h>
#include <klib/asm-offset.h>
#include <hal/riscv64.h>
#include <hal/rv64-platform.h>
#include <hal/hal-traps.h>

#if defined(CONFIG_HAL)
ASMMAC_BOOT_TEXT_SECTION
#else
ASMMAC_TEXT_SECTION
#endif  /* CONFIG_HAL */

/**
    マシンモード割込みベクタ
    void machine_mode_vector相当のサブルーチン
 */
ASMMAC_BALIGN(RV64_VECTORBASE_ALIGN)	
#if defined(CONFIG_HAL)
ASMMAC_BOOT_FUNCTION(machine_mode_vector)
#else
ASMMAC_FUNCTION(machine_mode_vector)
#endif  /*  CONFIG_HAL  */
        csrrw t0, mscratch, t0
	/* これ以降t0以外のレジスタを使用する
	 */
	sd   sp, MSCRATCH_SAVED_SP(t0)  /* スタックを保存する                   */
	ld   sp, MSCRATCH_MSTACK_SP(t0) /* マシンモードスタックをロードする     */
	addi sp, sp, -RV64_TRAP_CONTEXT_SIZE /* コンテキスト保存領域を確保する */

	/*
	 * epc, sp, mstaus/sstatus, t0以外のレジスタを保存する
	 */
	RV64_ASM_SAVE_CONTEXT_COMMON(sp)
	
	csrr s1, mepc                      /* 割込み元アドレス       */
	sd   s1, RV64_TRAP_CONTEXT_EPC(sp) /* 割込み元アドレスを退避 */

	csrr s1, mstatus                       /* 例外発生時のmstatus  */
	sd   s1, RV64_TRAP_CONTEXT_ESTATUS(sp) /* mstatusを退避        */

	ld   s1, MSCRATCH_SAVED_SP(t0)	   /* 割込み元スタック       */
	sd   s1, RV64_TRAP_CONTEXT_SP(sp)  /* 割込み元スタックを退避 */

	mv   s1, t0                        /* mscratch情報           */
	
        csrrw t0, mscratch, t0             /* t0レジスタ             */
	sd   t0, RV64_TRAP_CONTEXT_T0(sp)  /* t0レジスタを退避       */

	/*
	 *  コンテキスト保存処理完了
	 */
	ld   tp, SSCRATCH_CPUINF(s1)       /* CPU情報をtpにロード    */

	csrr  s2, mip      /* MIPレジスタを読み込む    */
	li    s3, MIP_MTIP /* マシンモードタイマ割込み */
	and   s4, a2, a1   /* mip & MIP_MTIPを計算     */
	beqz  s4, skip_mtimer /* ビットが立っていなければタイマ更新処理をスキップする */
	
	/* これ以降以下のようにレジスタを使用する
	   s1  mscratch情報物理アドレス
	   s2  自hart MTIME_CMPレジスタ
	   s3  MTIMEレジスタ
	   s4  タイマインターバル
	 */
	ld   s2, MSCRATCH_MTIMECMP_PADDR(s0)
	ld   s3, MSCRATCH_MTIME_PADDR(s0)
	ld   s4, MSCRATCH_TIMER_INTERVAL(s0)

	ld   s5, (s3)    /* 現在のMTIMEを読み込み  */
	add  s6, s5, s4  /* 次のタイマ設定値を算出 */
	sd   s6, (s3)    /* 次のタイマ設定値を設定 */

	li   s5, MIP_SSIP /* スーパーバイザモードソフトウエア割込み発生 */
        csrw sip, s5

skip_mtimer:
	/*
	 * コンテキスト復元
	 */
	ld   s1, RV64_TRAP_CONTEXT_ESTATUS(sp) /* mstatus  */
	csrw mstatus, s1                       /* mstatusを復元        */

	ld   s2, RV64_TRAP_CONTEXT_EPC(sp) /* 割込み元アドレス       */
	csrw mepc, s2                      /* 割込み元アドレスを設定 */	

	/*
	 * sp以外のレジスタを復元する
	 */
	RV64_ASM_RESTORE_CONTEXT_COMMON(sp)
	ld     sp, RV64_TRAP_CONTEXT_SP(sp)  /* スタックを復元する */
        mret
	
/**
    マシンモード関連の設定
    void setup_machine_mode(uint64_t hartid, mscratch_info *mscratch_tbl)相当のサブルーチン
    @param[in] hartid       自hartのID (a0レジスタ)
    @param[in] mscratch_tbl mscratch情報テーブルの先頭物理アドレス
 */
#if defined(CONFIG_HAL)
ASMMAC_BOOT_FUNCTION(setup_machine_mode)
#else
ASMMAC_FUNCTION(setup_machine_mode)
#endif  /*  CONFIG_HAL  */
	addi sp,sp,-16   /*  レジスタ2つ分のアドレスを確保する               */
	sd   ra, 8(sp)   /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 16  /*  fpを更新する                                    */

	li   t1, MSCRATCH_MSCRATCH_INFO_SIZE /* mscratch情報のサイズをロード */
	mul  t1, t1, a0           /* 自hartのmscratch情報オフセットアドレスを算出  */
	add  t2, a1, t1           /* 自hartのmscratch情報アドレスを算出  */
	li   a2, RV64_CLINT_PADDR /* CLINT物理アドレスをロード          */

	/* これ以降では以下のようにレジスタを使用する
	   a0 hartid ( boot.Sで獲得した値を破壊しない )
	   a2 CLINTレジスタの物理アドレス
	   t2 自hartのmscratch情報アドレス (物理アドレス)
	*/
        la a3, _mint_stack_end /* マシンモードスタック領域の末尾を参照       */
	li a4, KC_ISTACK_SIZE /* 割込みスタックサイズ                        */
	mul a5, a4, a0        /* 自hartスタックの底へのオフセットを計算      */
	sub a4, a3, a5        /* 自hartのスタックの底を算出                  */

	sd  a4, MSCRATCH_MSTACK_SP(t2) /* マシンモードスタックを設定         */

	li  a4, RV64_CLINT_MTIMECMP_OFFSET   /* MTIME_CMPレジスタオフセットアドレス */
	li  a5, RV64_CLINT_MTIMECMP_SIZE     /* MTIME_CMPレジスタサイズ */
	mul a5, a5, a0                       /* 自hart MTIME_CMPレジスタオフセット       */
	add a3, a2, a4                       /* MTIME_CMPレジスタアドレス                */
	add a3, a3, a5                       /* 自hartMTIME_CMPレジスタアドレス          */
	sd  a3, MSCRATCH_MTIMECMP_PADDR(t2)  /* 自hart MTIME_CMPレジスタアドレスをセット */

	li  a4, RV64_CLINT_MTIME_OFFSET      /* MTIMEレジスタオフセットアドレス */
	add a3, a2, a4                       /* MTIMEレジスタアドレス           */
	sd  a3, MSCRATCH_MTIME_PADDR(t2)     /* MTIMEレジスタアドレスをセット   */

	li  a4, RV64_CLINT_MTIME_INTERVAL    /* タイマインターバル           */
	sd  a4, MSCRATCH_TIMER_INTERVAL(t2)  /* タイマインターバルをセット   */

	csrw mscratch, t2                    /* mscratchレジスタ値を設定     */

	la   a3, machine_mode_vector         /* マシンモードベクタアドレスをロード  */
	csrw mtvec, a3                       /* マシンモードベクタアドレスを設定    */
	
	ld s0, 0(sp)     /*  s0レジスタを復元する                            */
	ld ra, 8(sp)     /*  raレジスタを復元する                            */
	addi sp,sp,16    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */
	
#if defined(CONFIG_HAL)
ASMMAC_BOOT_DATA_SECTION
#else
ASMMAC_DATA_SECTION
#endif  /* CONFIG_HAL */	

	
