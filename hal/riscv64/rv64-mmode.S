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
#include <hal/rv64-platform.h>
#include <hal/rv64-pgtbl.h>

#if defined(CONFIG_HAL)
ASMMAC_BOOT_TEXT_SECTION
#else
ASMMAC_TEXT_SECTION
#endif  /* CONFIG_HAL */

/**
    マシンモード割込みベクタ
    void machine_mode_vector相当のサブルーチン
 */
#if defined(CONFIG_HAL)
ASMMAC_BOOT_FUNCTION(machine_mode_vector)
#else
ASMMAC_FUNCTION(machine_mode_vector)
#endif  /*  CONFIG_HAL  */

/**
    マシンモード関連の設定
    void setup_machine_mode(hartid)相当のサブルーチン
    @param[in] hartid 自hartのID (a0レジスタ)
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
	add  t2, a3, t1           /* 自hartのmscratch情報アドレスを算出  */
	li   a1, RV64_CLINT_PADDR /* CLINT物理アドレスをロード          */

	/* これ以降では以下のようにレジスタを使用する
	   a0 hartid ( boot.Sで獲得した値を破壊しない )
	   a1 CLINTレジスタの物理アドレス
	   t2 自hartのmscratch情報アドレス (物理アドレス)
	*/
        la a3, _mint_stack_end /* マシンモードスタック領域の末尾を参照       */
	li a4, KC_ISTACK_SIZE /* 割込みスタックサイズ                        */
	mul a5, a4, a0        /* 自hartスタックの底へのオフセットを計算      */
	sub a4, a3, a5        /* 自hartのスタックの底を算出                  */

	sd  a4, MSCRATCH_MSTACK_SP(t2) /* マシンモードスタックを設定         */
	sd  a0, MSCRATCH_HARTID(t2)    /* 自hartidを設定                     */	

	li  a4, RV64_CLINT_MTIMECMP_OFFSET   /* MTIME_CMPレジスタオフセットアドレス */
	li  a5, RV64_CLINT_MTIMECMP_SIZE     /* MTIME_CMPレジスタサイズ */
	mul a5, a5, a0                       /* 自hart MTIME_CMPレジスタオフセット       */
	add a3, a1, a4                       /* MTIME_CMPレジスタアドレス                */
	add a3, a3, a5                       /* 自hartMTIME_CMPレジスタアドレス          */
	sd  a3, MSCRATCH_MTIMECMP_PADDR(t2)  /* 自hart MTIME_CMPレジスタアドレスをセット */

	li  a4, RV64_CLINT_MTIME_OFFSET      /* MTIMEレジスタオフセットアドレス */
	add a3, a1, a4                       /* MTIMEレジスタアドレス           */
	sd  a3, MSCRATCH_MTIME_PADDR(t2)     /* MTIMEレジスタアドレスをセット   */

	li  a4, RV64_CLINT_MTIME_INTERVAL       /* タイマインターバル           */
	sd  a4, MSCRATCH_TIMER_INTERVAL_CYC(t2) /* タイマインターバルをセット   */

	csrw mscratch, t2   /* mscratchレジスタ値を設定  */
	
	ld s0, 0(sp)     /*  s0レジスタを復元する                            */
	ld ra, 8(sp)     /*  raレジスタを復元する                            */
	addi sp,sp,16    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */
	
#if defined(CONFIG_HAL)
ASMMAC_BOOT_DATA_SECTION
#else
ASMMAC_DATA_SECTION
#endif  /* CONFIG_HAL */	

	
