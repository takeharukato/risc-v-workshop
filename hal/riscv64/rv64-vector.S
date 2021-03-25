/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 vector routines                                          */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1
#include <kern/kern-consts.h>
#include <klib/asm-macros.h>
#include <klib/asm-offset.h>
#include <hal/riscv64.h>
#include <hal/rv64-platform.h>
#include <hal/hal-traps.h>

ASMMAC_TEXT_SECTION

/**
    スーパバイザモード->スーパバイザモード割込みベクタ
    void supervisor_trap_vector(void)相当のサブルーチン
 */
ASMMAC_BALIGN(RV64_VECTORBASE_ALIGN)
ASMMAC_FUNCTION(supervisor_trap_vector)
	addi sp, sp, -RV64_TRAP_CONTEXT_SIZE /* コンテキスト保存領域を確保する */

	/*
	 * sp, t0, sepc, sstatus以外のレジスタを保存する
	 */
	RV64_ASM_SAVE_CONTEXT_COMMON(sp)
	/*
	 * sepc, sstatusレジスタを保存する
	 */
	RV64_ASM_SAVE_SUPERVISOR_CONTEXT_COMMON(sp)

	sd   t0, RV64_TRAP_CONTEXT_T0(sp)   /* t0レジスタを退避           */

	addi s1, sp, RV64_TRAP_CONTEXT_SIZE /* 割込み元スタックの値を算出 */
	sd   s1, RV64_TRAP_CONTEXT_SP(sp)   /* 割込み元スタックを退避     */

	jal  call_trap_common               /* 共通例外ハンドラ呼び出し処理へ  */

/**
    ユーザモード->スーパバイザモード割込みベクタ
    void user_trap_vector(void)相当のサブルーチン
 */
ASMMAC_BALIGN(RV64_VECTORBASE_ALIGN)
ASMMAC_FUNCTION(user_trap_vector)
        csrrw t0, sscratch, t0               /* sscratch(sscratch情報ポインタ)とt0を交換 */
	/* これ以降t0以外のレジスタを使用する
	 */
	sd   sp, SSCRATCH_SAVED_SP(t0)       /* 割込み元スタックを保存する     */
	ld   sp, SSCRATCH_SSTACK_SP(t0)      /* カーネルスタックをロードする   */
	addi sp, sp, -RV64_TRAP_CONTEXT_SIZE /* コンテキスト保存領域を確保する */

	/*
	 * sp, t0, sepc, sstatus以外のレジスタを保存する
	 */
	RV64_ASM_SAVE_CONTEXT_COMMON(sp)
	/*
	 * sepc, sstatusレジスタを保存する
	 */
	RV64_ASM_SAVE_SUPERVISOR_CONTEXT_COMMON(sp)

	ld   s1, SSCRATCH_SAVED_SP(t0)	   /* 割込み元スタック       */
	sd   s1, RV64_TRAP_CONTEXT_SP(sp)  /* 割込み元スタックを退避 */

	mv   s1, t0                        /* sscratch情報                      */

        csrrw t0, sscratch, t0             /* t0レジスタ/sscratchレジスタを復元 */
	sd   t0, RV64_TRAP_CONTEXT_T0(sp)  /* t0レジスタを退避                  */

	la   s2, supervisor_trap_vector /* スーパバイザモードベクタアドレスをロード */
	csrw stvec, s2                  /* スーパバイザモードベクタアドレスを設定   */

	ld   tp, SSCRATCH_HARTID(s1)       /* 物理CPUIDをtpにロード             */

call_trap_common:

	/*
	 * 例外ハンドラ呼び出し
	 */
	mv   a0, sp                        /* 例外コンテキスト              */
	csrr a1, scause                    /* 例外発生要因                  */
	csrr a2, stval                     /* Supervisor Trap Value         */

	call trap_common                   /* 共通例外ハンドラ呼び出し      */
/**
    void rv64_return_from_trap(void)相当のサブルーチン
 */
ASMMAC_FUNCTION(rv64_return_from_trap)
	/*
	 * コンテキスト復元
	 */
	ld   s1, RV64_TRAP_CONTEXT_EPC(sp) /* 割込み元アドレス       */
	csrw sepc, s1                      /* 割込み元アドレスを設定 */

	ld   s1, RV64_TRAP_CONTEXT_ESTATUS(sp) /* sstatus        */

	and  s2, s1, SSTATUS_SPP /* Supervisor Previous modeを参照                       */
	bne  s2, x0, to_kernel   /* ユーザモード復帰(SSTATUS_SPPが0)でなければスキップ   */

	/* ユーザコンテキスト復帰時の処理
	 * - ユーザモードからの割込みを有効化
	 * - ユーザモード->スーパバイザモード割込みベクタに割り込みベクタを設定する
	 */
	ld   tp, RV64_TRAP_CONTEXT_TP(sp)      /* tpレジスタを復元 */

	/* ユーザモード->スーパバイザモード割込みベクタをロード */
	la   s2, user_trap_vector
	csrw stvec, s2    /*  ベクタアドレスを設定 */

	or   s1, s1, SSTATUS_SPIE              /* ユーザモード割り込みを有効化 */

to_kernel:
	csrw sstatus, s1                       /* sstatusを復元  */

	/*
	 * sp, tp以外のレジスタを復元する
	 */
	RV64_ASM_RESTORE_CONTEXT_COMMON(sp)
	ld     sp, RV64_TRAP_CONTEXT_SP(sp)  /* スタックを復元する */
        sret

#if defined(CONFIG_HAL)
ASMMAC_BOOT_TEXT_SECTION
#else
ASMMAC_TEXT_SECTION
#endif  /* CONFIG_HAL */

/**
    割込みベクタの設定
    void setup_vector(uint64_t hartid, sscratch_info *sscratch_tbl)相当のサブルーチン
    @param[in] hartid 自hartのID (a0レジスタ)
    @param[in] sscratch_tbl sscratch情報テーブルの先頭仮想アドレス
 */
#if defined(CONFIG_HAL)
ASMMAC_BOOT_FUNCTION(setup_vector)
#else
ASMMAC_FUNCTION(setup_vector)
#endif  /*  CONFIG_HAL  */
	addi sp,sp,-16   /*  レジスタ2つ分のアドレスを確保する               */
	sd   ra, 8(sp)   /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 16  /*  fpを更新する                                    */

	li   t1, SSCRATCH_SSCRATCH_INFO_SIZE /* sscratch情報のサイズをロード */
	mul  t1, t1, a0           /* 自hartのsscratch情報オフセットアドレスを算出  */
	add  t0, a1, t1           /* 自hartのsscratch情報アドレスを算出  */

#if defined(CONFIG_UPPERHALF_KERNEL)
	li  t2, HAL_KERN_VMA_BASE
	sub t0, t0, t2            /* mscratchテーブルの物理アドレスを算出   */
#endif  /*  CONFIG_UPPERHALF_KERNEL  */

	/* これ以降では以下のようにレジスタを使用する
	   a0 hartid ( boot.Sで獲得した値を破壊しない )
	   t0 自hartのsscratch情報アドレス (物理アドレス)
	*/
        la a3, _intr_stack_end /* 割込みスタック領域の末尾を参照       */
	li a4, KC_ISTACK_SIZE  /* 割込みスタックサイズ                        */
	mul a5, a4, a0         /* 自hartスタックの底へのオフセットを計算      */
	sub a4, a3, a5         /* 自hartのスタックの底を算出                  */

	sd  a4, SSCRATCH_ISTACK_SP(t0) /* 割込みスタックを設定                */

        la a3, _bsp_stack_end  /* カーネルスタック領域の末尾を参照            */
	li a4, KC_KSTACK_SIZE  /* カーネルスタックサイズ                      */
	mul a5, a4, a0         /* 自hartスタックの底へのオフセットを計算      */
	sub a4, a3, a5         /* 自hartのスタックの底を算出                  */

	sd  a4, SSCRATCH_SSTACK_SP(t0) /* スーパバイザモードスタックを設定    */

	csrw sscratch, t0      /* sscratchレジスタ値を設定            */

	la   a2, supervisor_vector_addr
	ld   a2, (a2)    /* スーパバイザモードベクタアドレスをロード */

	csrw stvec, a2   /* スーパバイザモードベクタアドレスを設定   */

	ld s0, 0(sp)     /*  s0レジスタを復元する                            */
	ld ra, 8(sp)     /*  raレジスタを復元する                            */
	addi sp,sp,16    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */

#if defined(CONFIG_HAL)
ASMMAC_BOOT_DATA_SECTION
#else
ASMMAC_DATA_SECTION
#endif  /* CONFIG_HAL */
ASMMAC_BALIGN(16)
supervisor_vector_addr:
	.dword supervisor_trap_vector  /* supervisor_trap_vectorのアドレスを格納 */
