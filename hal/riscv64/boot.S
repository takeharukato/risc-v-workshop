/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V boot routine                                               */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE
#include <kern/kern-consts.h>
#include <klib/asm-macros.h>
#include <klib/asm-offset.h>
#include <kern/thr-preempt.h>
#include <hal/riscv64.h>
#include <hal/rv64-platform.h>
#include <hal/hal-stack.h>

#if defined(CONFIG_HAL)
ASMMAC_BOOT_TEXT_SECTION
#else
ASMMAC_TEXT_SECTION
#endif  /* CONFIG_HAL */

ASMMAC_EXTERN_SYMBOL(prepare)

#if defined(CONFIG_HAL)	
ASMMAC_BOOT_FUNCTION(entry)
#else
ASMMAC_FUNCTION(entry)
#endif  /* CONFIG_HAL */    
       /*
	* ブートスタック情報を算出する
	* a0 自hartのhartid (注意: 本サブルーチン内で使い続ける)
	*/
        csrr a0, mhartid      /* hartidを読み込む                        */

        la a1, _bsp_stack_end /* ブートスタック領域の末尾を参照          */
	li a2, KC_KSTACK_SIZE /* スタックサイズをa2に代入                */
	mul a6, a2, a0        /* BSPスタックの底から自hartスタックの底へのオフセットを計算 */
	sub a3, a1, a6        /* 自hartのスタックの底を算出              */
	sub a5, a3, a2        /* 自hartのスタックのトップを算出          */
	li  a7, TI_THREAD_INFO_SIZE /* スレッド情報をスタックの底に設定  */
	sub a4, a3, a7        /* a4がスレッド情報を指すように設定        */

	/* スレッド情報
         * 上記の処理でa3, a4, a5に以下の値が設定されている
	 * a3 自hartのスタックの底
	 * a4 スレッド情報領域のアドレス
	 * a5 自hartのスタックのトップ
	 */
	li  a2, TI_MAGIC              /* マジック番号読み込み            */
	sw  a2, TI_MAGIC_OFFSET(a4)   /* マジック番号書き込み            */
	sw  x0, TI_INTRCNT_OFFSET(a4) /* 割り込みカウント初期化          */
	sw  x0, TI_PREEMPT_OFFSET(a4) /* プリエンプションカウント初期化  */
	sw  x0, TI_FLAGS_OFFSET(a4)   /* フラグ初期化                    */
	sw  x0, TI_MDFLAGS_OFFSET(a4) /* アーキ依存フラグ初期化          */
	sw  x0, TI_CPU_OFFSET(a4)     /* 論理CPU番号初期化               */
	sw  a5, TI_KSTACK_OFFSET(a4)  /* カーネルスタック開始位置初期化  */
	sw  x0, TI_THR_OFFSET(a4)     /* 論理CPU番号初期化               */

	mv  sp, a4                    /*  スレッド情報位置にスタックポインタ設定     */

	bne  a0, x0, wait_bsp         /*  ページテーブル設定を待ち合わせる */
	
	/*
	 * BSSクリア/マシンモード情報テーブル/MMU設定 (BSPのみ実施)
	 */
	li a2, HAL_KERN_VMA_BASE  /* カーネル仮想アドレスオフセット   */

	/*
	 * BSSクリア (BSS中にあるマシンモード情報テーブルを設定する前に実施)
	 */
	la a3, bss_start_addr
	ld a3, (a3)
#if defined(CONFIG_UPPERHALF_KERNEL)
	sub a3, a3, a2            /* BSS開始物理アドレスを算出   */
#endif  /*  CONFIG_UPPERHALF_KERNEL  */

	la a4, bss_end_addr
	ld a4, (a4)
#if defined(CONFIG_UPPERHALF_KERNEL)
	sub a4, a4, a2            /* BSS終了物理アドレスを算出   */
#endif  /*  CONFIG_UPPERHALF_KERNEL  */
	mv a5, a3                 /* 書き込み先アドレス */
clear_bss:
	sd x0, (a5)               /* 0クリア                       */
	add a5, a5, 8             /* アドレス加算                  */
	blt a5, a4, clear_bss     /* BSSの終端に達するまで繰り返す */

	/*
	 * マシンモード情報テーブルのアドレスを設定
	 */
	la a3, mscratch_tbl_addr  /* mscratchテーブルのアドレスを格納した変数のアドレスをロード */
	ld a1, (a3)               /* mscratchテーブルのアドレスをロード */
#if defined(CONFIG_UPPERHALF_KERNEL)
	sub a1, a1, a2            /* mscratchテーブルの物理アドレスを算出   */
	sd  a1, (a1)              /* mscratchテーブルの物理アドレスをストア */
#endif  /*  CONFIG_UPPERHALF_KERNEL  */
	
	/*
	 * MMU設定
	 */
	call setup_pre_pgtbl
	call load_pre_pgtbl

ap_boot: /* APの処理再開位置 */
	/* 第1引数にhart, 第2引数にmscratchテーブルの物理アドレスを指定 */ 
	call setup_machine_mode  /* マシンモードの設定を実施 */

	/*
	 * 割込みベクタのセットアップ
	 */
	la a1, sscratch_tbl_addr  /* sscratchテーブルの仮想アドレスをロード */
	ld a1, (a1)
	/* 第1引数にhart, 第2引数にsscratchテーブルの物理アドレスを指定 */ 
	call setup_vector  /* 割込みベクタの設定 */

	/*
	 * 割込み/例外の回送
	 */
	/* 例外をスーパバイザモードに回送
	 */
	li a2, MEDELEG_MASK
	csrw medeleg, a2
	/* 割込みをスーパバイザモードに回送
	 */
	li a2, MIDELEG_MASK
	csrw mideleg, a2
	/* スーパバイザモードに移行し, Cのエントリ関数(prepare)に移行する
	 */
	csrr a1, mstatus          /* mstatusを読み込む */
	li a2, ~MSTATUS_MPP_MASK  /* Machine Previous Privilege Modeビットをクリアする */
	and a1, a1, a2
	li a2, MSTATUS_MPP_S  /* Machine Previous Privilege Modeを supervisor modeに設定 */
	or a1, a1, a2         
	csrw mstatus, a1      /* mstatusを更新                                     */

#if defined(CONFIG_UPPERHALF_KERNEL)
	/* スタックをアッパーハーフマッピングに合わせて再設定
	 */
	li a1, HAL_KERN_VMA_BASE  /* カーネル仮想アドレスオフセット   */
	add sp, sp, a1
#endif  /*  CONFIG_UPPERHALF_KERNEL  */

	la   t0, c_entry_addr /* Cのエントリ関数アドレス格納変数のアドレスをロード */
	ld   a1, (t0)         /* Cのエントリ関数のアドレスをロード                 */
	csrw mepc, a1         /* Machine Exception Program Counterを更新           */
	mret                  /* マシンモードからの復帰によりスーパバイザモードに遷移 */
junk:
        j junk
wait_bsp:
	wfi                     /* CPUを休眠する */
	j wait_bsp

#if defined(CONFIG_HAL)
ASMMAC_BOOT_DATA_SECTION
#else
ASMMAC_DATA_SECTION
#endif  /* CONFIG_HAL */	
/* カーネルのCエントリ関数のアドレスを格納する領域
 * RISC-V64の場合, la疑似命令で読み込むアドレスは, 符号付き32bit値でなければならない
 * RISC-V Assembly Programmer's Manual
 * https://github.com/riscv/riscv-asm-manual/blob/master/riscv-asm.md
 * 参照。
 *
 * メモリモデルがmedanyの場合, プログラムカウンタ(pc)からの相対アドレス指定となり,
 * la a0, symbol
 * は, 
 * 1:    auipc a0,     %pcrel_hi(symbol) # load symbol(hi)
 *       addi  a0, a0, %pcrel_lo(1b)     # load symbol(lo)
 * として扱われ, medlowの場合, 絶対アドレス指定となり,
 *       lui  a0,      %hi(symbol)       # load symbol(hi)
 *       addi a0, a0,  %lo(symbol)       # load symbol(lo)
 * として扱われる。
 * いずれの場合も, symbolのアドレスは, 符号付き32bit値でなければならない.
 * しかしながら, アッパーハーフマッピングの場合のカーネルアドレスは, 
 * 符号付き32bit値の範囲内にないのでリンク時にpc相対で32bit範囲内に配置される
 * 大域変数にアドレスを格納しておき大域変数のアドレスをロードし, その内容を
 * ld 命令で読み込むことで絶対アドレスを取得するようにしている。
 */
ASMMAC_DECLARE_NAME(c_entry_addr)
ASMMAC_BALIGN(8)
c_entry_addr:
	.dword prepare       /* prepare関数のアドレスを格納 */

ASMMAC_BALIGN(16)
mscratch_tbl_addr:
	.dword mscratch_tbl  /* mscratch_tblのアドレスを格納 */

ASMMAC_BALIGN(16)
sscratch_tbl_addr:
	.dword sscratch_tbl  /* sscratch_tblのアドレスを格納 */

ASMMAC_BALIGN(16)
bss_start_addr:
	.dword __bss_start   /* bss開始アドレスを格納  */

ASMMAC_BALIGN(16)
bss_end_addr:
	.dword __bss_end     /* bss終了アドレスを格納  */

