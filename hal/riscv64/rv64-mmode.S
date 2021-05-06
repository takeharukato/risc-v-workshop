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
	csrrw t0, mscratch, t0  /* t0とmscratchを交換 */

	/* t0以外のレジスタを使用してコンテキストを保存	*/
	sd   sp, MSCRATCH_SAVED_SP(t0)  /* スタックを保存する                   */
	ld   sp, MSCRATCH_MSTACK_SP(t0) /* マシンモードスタックをロードする     */
	addi sp, sp, -RV64_TRAP_CONTEXT_SIZE /* コンテキスト保存領域を確保する */

	/*
	* mepc, sp, mstaus, t0以外のレジスタを保存する
	*/
	RV64_ASM_SAVE_CONTEXT_COMMON(sp)

	/* mepc, sp, mstaus, t0レジスタを保存する
	* t0 t0以外のコンテキストを退避するためにmscratch情報物理アドレスを指している
	* t1 スクラッチに使用
	* t5 mscratch情報物理アドレスの
	*/

	csrr t1, mepc                      /* 割込み元アドレス       */
	sd   t1, RV64_TRAP_CONTEXT_EPC(sp) /* 割込み元アドレスを退避 */

	csrr t1, mstatus                       /* 割込み発生時のmstatus  */
	sd   t1, RV64_TRAP_CONTEXT_ESTATUS(sp) /* mstatusを退避        */

	ld   t1, MSCRATCH_SAVED_SP(t0)     /* 割込み元スタック       */
	sd   t1, RV64_TRAP_CONTEXT_SP(sp)  /* 割込み元スタックを退避 */

	mv   t5, t0                        /* mscratch情報           */

	/* これ以降t5はmscratch情報物理アドレスを保持する用途で使用      */
	csrrw t0, mscratch, t0             /* t0レジスタとmscratchを交換 */
	sd   t0, RV64_TRAP_CONTEXT_T0(sp)  /* t0レジスタを退避           */

	li   t6, SIP_SSIP  /* スーパーバイザモードソフトウエア割込み発生 */

	/* 以降のレジスタ使用用途
	t0  スクラッチに使用
	t1  スクラッチに使用
	t2  例外種別判定の時は未使用,
	    タイマ処理の時は, 自hart MTIME_CMPレジスタ
	t3  例外種別判定の時は未使用,
	    タイマ処理の時は, MTIMEレジスタ
	t4  例外種別判定の時は, macuse（例外要因種別),
	    IPI割込み処理の時は, 自hart MSIPレジスタ
	    タイマ処理の時は, タイマインターバル
	t5  mscratch情報物理アドレス
	t6  スーパーバイザモードソフトウエア割込み発生ビットを保持
	*/

	/* 例外種別判定処理
	t0  スクラッチに使用
	t1  スクラッチに使用
	t2  未使用
	t3  未使用
	t4  macuse（例外要因種別)
	t5  mscratch情報物理アドレス
	t6  スーパーバイザモードソフトウエア割込み発生ビットを保持
	*/
	csrr  t4, mcause        /* 例外要因種別読み込み */
	li    t0, MCAUSE_INTR
	and   t1, t4, t0        /* マシンモードへの割込みか?     */
	bnez  t1, handle_mintr  /* マシンモードへの割込みの場合, IPI/タイマ更新処理へ */

	/* ecall命令/不正例外の処理
	t0  スクラッチに使用
	t1  未使用
	t2  未使用
	t3  未使用
	t4  例外発生要因
	t5  mscratch情報物理アドレス
	t6  スーパーバイザモードソフトウエア割込み発生ビットを保持
	*/

	/* マシンモードへのecallまたは不正例外処理
	 * マシンモード例外処理を呼び出すための引数を設定
	 */
	mv   a0, sp                        /* 例外コンテキスト              */
	csrr a1, mcause                    /* 例外発生要因                  */
	csrr a2, mtval                     /* Machine Trap Value            */

        addi sp, sp, -8                    /* mcause/mtval保存域を用意する */
	sd   a1, 0(sp)                     /* 例外要因保存                 */

	/*
	 * TODO: マシンモード例外処理(ecall処理含む)を実装
	 */

	ld   t4, 0(sp)                     /* 例外要因復元                */
	addi sp, sp, 8                     /*  スタック位置を復元         */

        li    t0, MCAUSE_ENVCALL_SMODE  /* マシンモードへのecall */
	and   t1, t4, t0        /* マシンモードへのecallか?     */
	beqz  t1, illegal_machine_mode_exception /* IPI/タイマ/ecallのいずれでもない */

	/* ecallの場合は, ecall命令の次の命令を指して抜ける */
	ld    t0, RV64_TRAP_CONTEXT_EPC(sp) /* コンテキスト内の割込み元アドレス取得     */
	add   t0, t0, 4                     /* ecall命令の次の命令を指す                */
	sd    t0, RV64_TRAP_CONTEXT_EPC(sp) /* 割込み元アドレスを更新                   */
	j     rv64_return_from_machine_mode /* 例外出口処理へ移行                       */

	/* IPI割込み処理
	t0  スクラッチに使用
	t1  スクラッチに使用
	t2  未使用
	t3  未使用
	t4  自hart MSIPレジスタ
	t5  mscratch情報物理アドレス
	t6  スーパーバイザモードソフトウエア割込み発生ビットを保持
	*/
handle_mintr:
	ld    t4, MSCRATCH_MSIP_PADDR(t5)    /* 自hart MSIPレジスタ                      */
	lw    t1, (t4)                       /* MIPレジスタを読み込み                    */
	and   t1, t1, 0x1                    /* プロセッサ間割込みが上がっているか?      */
	beqz  t1, handle_mtimer              /* ビットが立っていなければタイマ処理へ     */
	sw    x0, (t4)                       /* プロセッサ間割込み受け付け通知           */

	mv   a0, sp                        /* 例外コンテキスト              */
	csrr a1, mcause                    /* 例外発生要因                  */
	csrr a2, mtval                     /* Machine Trap Value            */
	/* TODO: IPI割込み処理  */

	j     rv64_return_from_machine_mode  /* 例外出口処理へ移行          */

	/* タイマ処理
	t0  スクラッチに使用
	t1  スクラッチに使用
	t2  自hart MTIME_CMPレジスタ
	t3  自hart MTIMEレジスタ
	t4  自hartタイマインターバル
	t5  mscratch情報物理アドレス
	t6  スーパーバイザモードソフトウエア割込み発生ビットを保持
	*/
handle_mtimer:
	csrr  t0, mip      /* MIPレジスタを読み込む    */
	li    t1, MIP_MTIP /* マシンモードタイマ割込み */
	and   t0, t0, t1   /* マシンモードタイマ割込みの発生を確認     */
	beqz  t0, rv64_return_from_machine_mode /* マシンモードタイマ割込みでなければ抜ける */

	ld   t2, MSCRATCH_MTIMECMP_PADDR(t5)  /* 自hart MTIME_CMPレジスタ  */
	ld   t3, MSCRATCH_MTIME_PADDR(t5)     /* 自hart MTIMEレジスタ      */
	ld   t4, MSCRATCH_TIMER_INTERVAL(t5)  /* 自hart タイマインターバル */

	ld   t0, (t3)                        /* 現在のMTIMEを読み込み     */
	sd   t0, MSCRATCH_LAST_TIME_VAL(t5)  /* タイマ設定時のMTIMEを記録 */
	add  t1, t0, t4                      /* 次のタイマ設定値を算出    */
	sd   t1, (t2)                        /* 次のタイマ設定値を設定    */

	li   t0, SIP_STIP /* スーパーバイザモードタイマ割込み発生 */
	or   t6, t6, t0   /* タイマ割込み通知 */

	/* 割込み出口処理
	t0  スクラッチに使用
	t1  スクラッチに使用
	t2  未使用
	t3  未使用
	t4  未使用
	t5  mscratch情報物理アドレス
	t6  スーパーバイザモードソフトウエア割込み発生ビットを保持
	*/
rv64_return_from_machine_mode:
	csrw sip, t6      /* スーパバイザ割込み要因設定 */

	/*
	* コンテキスト復元
	*/
	ld   t0, RV64_TRAP_CONTEXT_ESTATUS(sp) /* mstatus  */
	csrw mstatus, t0                       /* mstatusを復元        */

	ld   t0, RV64_TRAP_CONTEXT_EPC(sp) /* 割込み元アドレス       */
	csrw mepc, t0                      /* 割込み元アドレスを設定 */

	/*
	* sp以外のレジスタを復元する
	*/
	RV64_ASM_RESTORE_CONTEXT_COMMON(sp)
	ld     sp, RV64_TRAP_CONTEXT_SP(sp)  /* スタックを復元する */

	mret  /* 例外から復帰 */

illegal_machine_mode_exception:
	j	illegal_machine_mode_exception /* ここには来ないはず */

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
	sd   ra, 8(sp)   /*  raレジスタを退避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを退避する                            */
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
	add a3, a3, a5                       /* 自hart MTIME_CMPレジスタアドレス         */
	sd  a3, MSCRATCH_MTIMECMP_PADDR(t2)  /* 自hart MTIME_CMPレジスタアドレスをセット */

	li  a4, RV64_CLINT_MSIP_OFFSET       /* MSIPレジスタオフセットアドレス      */
	li  a5, RV64_CLINT_MSIP_SIZE         /* MSIPレジスタサイズ                  */
	mul a5, a5, a0                       /* 自hart MSIPレジスタオフセット       */
	add a3, a2, a4                       /* MSIPレジスタアドレス                */
	add a3, a3, a5                       /* 自hart MSIPレジスタアドレス         */
	sd  a3, MSCRATCH_MSIP_PADDR(t2)      /* 自hart MSIPレジスタアドレスをセット */

	li  a4, RV64_CLINT_MTIME_OFFSET      /* MTIMEレジスタオフセットアドレス */
	add a3, a2, a4                       /* MTIMEレジスタアドレス           */
	sd  a3, MSCRATCH_MTIME_PADDR(t2)     /* MTIMEレジスタアドレスをセット   */

	li  a4, RV64_CLINT_MTIME_INTERVAL    /* タイマインターバル           */
	sd  a4, MSCRATCH_TIMER_INTERVAL(t2)  /* タイマインターバルをセット   */

	/*
	 * タイマをセット
	 */
	ld  a5, MSCRATCH_MTIME_PADDR(t2)     /* MTIMEレジスタのアドレスを取得 */
	ld  a5, (a5)                         /* 現在のMTIME値を取得          */
	sd  a5, MSCRATCH_BOOT_TIME_VAL(t2)   /* ブート時のMTIME値を保存      */
	li  a6, RV64_CLINT_MTIME_INTERVAL    /* タイマインターバル           */
	add a5, a5, a6                       /* 次のタイマ値を算出           */
	ld  a6, MSCRATCH_MTIMECMP_PADDR(t2)  /* 自hart MTIME_CMPレジスタアドレスをロード */
	sd  a5, (a6)                         /* 次のタイマ値を設定           */

	csrw mscratch, t2                    /* mscratchレジスタ値を設定     */

	la   a3, machine_mode_vector         /* マシンモードベクタアドレスをロード  */
	csrw mtvec, a3                       /* マシンモードベクタアドレスを設定    */

	csrr a3, mie                         /* Machine interrupt enableレジスタをロード */
	li   a4, MIE_MTIE                    /* マシンモードタイマ割込み                 */
	or   a3, a3, a4                      /* マシンモードタイマ割込み許可を追加       */
	csrw mie, a3                         /* マシンモードタイマ割込み許可を有効化     */

	csrr a3, mstatus                     /* mstatusレジスタをロード  */
	li   a4, MSTATUS_MIE                 /* マシンモード割込み許可値 */
	or   a3, a3, a4                      /* マシンモード割込み許可を追加 */
	csrw mstatus, a3                     /* マシンモード割込み許可 */

	/*
	 * サイクルカウンタ命令の有効化
         */
	li a3, COUNTEREN_VAL
	csrw  mcounteren, a3   /* マシンモードカウンタイネーブル       */

	/*
	 * 割込み/例外の回送
	 */
	/* 例外処理をスーパバイザに委任
	 */
	li a3, MEDELEG_MASK
	csrw medeleg, a3
	/* 割込み処理をスーパバイザに委任
	 */
	li a3, MIDELEG_MASK
	csrw mideleg, a3

	ld s0, 0(sp)     /*  s0レジスタを復元する                            */
	ld ra, 8(sp)     /*  raレジスタを復元する                            */
	addi sp,sp,16    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */

#if defined(CONFIG_HAL)
ASMMAC_BOOT_DATA_SECTION
#else
ASMMAC_DATA_SECTION
#endif  /* CONFIG_HAL */
