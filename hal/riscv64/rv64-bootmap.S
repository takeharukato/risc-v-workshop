/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 boot map                                                 */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

#include <klib/asm-macros.h>
#include <hal/rv64-platform.h>
#include <hal/rv64-pgtbl.h>

#if defined(CONFIG_HAL)
ASMMAC_BOOT_TEXT_SECTION
#else
ASMMAC_TEXT_SECTION
#endif  /* CONFIG_HAL */

/**
    アッパーハーフカーネル用ページテーブルをロードする
    void load_pre_pgtbl(void) 相当のサブルーチン
 */
#if defined(CONFIG_HAL)
ASMMAC_BOOT_FUNCTION(load_pre_pgtbl)
#else
ASMMAC_FUNCTION(load_pre_pgtbl)
#endif  /*  CONFIG_HAL  */
	addi sp,sp,-16   /*  レジスタ2つ分のアドレスを確保する               */
	sd   ra, 8(sp)   /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 16  /*  fpを更新する                                    */
	
	/* ページテーブルをロードする
	 * レジスタ使用用途 (boot.Sでa0はhartid用に使用しているので使わない)
	 *  a1 VPN2テーブルの物理アドレス
	 *  a2 ページフレーム番号のマスク
	 *  a3 stapレジスタのモード値, ASID設定値
	 *  t0 一時レジスタ
	 *  t1 一時レジスタ
	 *  t2 一時レジスタ
	 */
	la  a1, pre_vpn2                   /* VPN2ページテーブル                  */
	li  a2, RV64_PPN_MASK              /* ページフレーム番号マスク            */
	li  a3, RV64_SATP_VAL(RV64_SATP_MODE_SV39, 0, 0) /* SATPレジスタモード値  */
	srli t0, a1, HAL_PAGE_SHIFT        /* VPN1テーブルのページ番号算出        */
	and  t0, t0, a2                    /* ページ番号をマスク                  */
	slli t0, t0, RV64_SATP_PPN_SHIFT   /* STAPレジスタ内のページ番号値を算出  */
	or   t0, t0, a3                    /* モードフラグを設定する              */
	csrw satp, t0                      /* ページテーブルを読み込む            */
	sfence.vma zero, zero              /* TLBをフラッシュする                 */

	ld s0, 0(sp)     /*  s0レジスタを復元する                            */
	ld ra, 8(sp)     /*  raレジスタを復元する                            */
	addi sp,sp,16    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */

/**
    ブート時のカーネルページテーブルをセットアップする
    void setup_pre_pgtbl(void) 相当のサブルーチン
 */
#if defined(CONFIG_HAL)
ASMMAC_BOOT_FUNCTION(setup_pre_pgtbl)
#else	
ASMMAC_FUNCTION(setup_pre_pgtbl)
#endif  /*  CONFIG_HAL  */
	addi sp,sp,-16   /*  レジスタ2つ分のアドレスを確保する               */
	sd   ra, 8(sp)   /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 16  /*  fpを更新する                                    */

	csrw satp, x0  /* アドレス変換を無効化                             */
	
       /*2MiBのページサイズで物理ページをマップするためのVPN1テーブルを作成する
	*
	* レジスタ使用用途 (boot.Sでa0はhartid用に使用しているので使わない)
	*  a1からa3まではループ中で不変
	*  a1 ページテーブルエントリの属性設定値 (pteの属性値)
	*  a2 VPN1テーブルの最終アドレス(最終アドレスの次のアドレス)
	*  a3 ページマップサイズ
	*  a4 マップ対象物理アドレス	
	*  a5 操作対象ページテーブルエントリのアドレス
	*  a6 一時レジスタ
	*/
	li a1, RV64_PTE_V|RV64_PTE_R|RV64_PTE_W|RV64_PTE_X|RV64_KERN_PTE_FLAGS /* フラグ値 */
	la a5, pre_vpn1  /* a5に操作対象VPN1テーブルエントリアドレスを設定 */
	mv a2, a5 /* a2にテーブルアドレスをセット */
	/* 最終アドレスオフセット */
	li a6, (RV64_BOOT_PGTBL_VPN1_NR * RV64_PGTBL_ENTRIES_NR * RV64_PGTBL_ENTRY_SIZE)
	add a2, a2, a6 /* 最終アドレスを算出 */
	li a3, HAL_PAGE_SIZE_2M  /* ページサイズ */
	li a4, HAL_KERN_PHY_BASE /* 物理アドレス */
vpn1_setup_loop:
	srli a6, a4, HAL_PAGE_SHIFT        /* ページフレーム番号算出      */
	slli a6, a6, RV64_PTE_PA_SHIFT     /* PTE内ページフレーム番号算出 */
	or   a6, a6, a1                    /* PTEフラグ値を設定           */
	sd   a6, (a5)                      /* ページテーブルに格納        */
	add  a5, a5, RV64_PGTBL_ENTRY_SIZE /* ページテーブルエントリ位置を更新 */
	add  a4, a4, a3                    /* 物理ページ位置を更新 */	
	blt  a5, a2, vpn1_setup_loop       /* 最終アドレスでなければ次のエントリを設定 */

	/* VPN2テーブルを作成する
	*
	* レジスタ使用用途 (boot.Sでa0はhartid用に使用しているので使わない)
	*  a1からa5まではループ中で不変
	*  a1 ページテーブルエントリの属性設定値 (pteの属性値)
	*  a2 最大ループ回数
        *  a3 VPN1ページテーブルのサイズ
	*  a4 ページテーブルエントリサイズ
	*  a5 ページフレーム番号のマスク
	*  a6 上位アドレス側の仮想アドレスへのオフセット
	*  a7 下位アドレス側の仮想アドレス
	*  t0 ループインデクス
	*  t1 VPN1ページテーブルエントリのアドレス
	*  t2 一時レジスタ
	*  t3 一時レジスタ
	*  t4 一時レジスタ
	*/
	li a1, RV64_PTE_V|RV64_KERN_PTE_FLAGS /* ページテーブル用フラグ値     */
	li a2, RV64_BOOT_PGTBL_VPN1_NR        /* VPN1テーブル数               */
	li a3, HAL_PAGE_SIZE                  /* ページサイズ                 */
	li a4, RV64_PGTBL_ENTRY_SIZE          /* ページテーブルエントリサイズ */
	li a5, RV64_PPN_MASK                  /* ページフレーム番号マスク     */
	li a6, HAL_KERN_VMA_BASE              /* カーネル仮想アドレス         */
	li a7, HAL_KERN_PHY_BASE              /* 物理ページ開始アドレス       */
	mv t0, x0                             /* ループインデクス             */
	la t1, pre_vpn1                       /* VPN1ページテーブル           */
vpn2_setup_loop:
	/* VPN1テーブルへのPTE値を算出
	 */
	srli t2, t1, HAL_PAGE_SHIFT        /* VPN1テーブルのページフレーム番号算出 */
	and  t2, t2, a5                    /* ページ番号をマスク                   */
	slli t2, t2, RV64_PTE_PA_SHIFT     /* PTE内ページフレーム番号算出          */
	or   t2, t2, a1                    /* PTEフラグ値を設定                    */
	
	/* 下位アドレス側のテーブル設定
	 */
	srli t3, a7, RV64_PGTBL_VPN2_INDEX_SHIFT /* 仮想アドレスからVPN2インデクスを算出 */
	and  t3, t3, RV64_PGTBL_VPN2_INDEX_MASK  /* VPN2インデクスをマスク               */
	mul  t3, t3, a4                          /* VPN2オフセットを設定                 */
	la   t4, pre_vpn2                        /* VPN2テーブルのアドレスをロード       */
	add  t4, t4, t3                          /* VPN2テーブルのPTEアドレス算出        */
	sd   t2, (t4)                            /* PTEを更新                            */

#if defined(CONFIG_UPPERHALF_KERNEL)
	/* 上位アドレス側のテーブル設定
	 */
	add  t3, a7, a6                          /* 上位仮想アドレスを算出               */
	srli t3, t3, RV64_PGTBL_VPN2_INDEX_SHIFT /* 仮想アドレスからVPN2インデクスを算出 */
	and  t3, t3, RV64_PGTBL_VPN2_INDEX_MASK  /* VPN2インデクスをマスク               */
	mul  t3, t3, a4                          /* VPN2オフセットを設定                 */
	la   t4, pre_vpn2                        /* VPN2テーブルのアドレスをロード       */
	add  t4, t4, t3                          /* VPN2テーブルのPTEアドレス算出        */
	sd   t2, (t4)                            /* PTEを更新                            */
#endif  /*  CONFIG_UPPERHALF_KERNEL  */
	
	li   t2, HAL_PAGE_SIZE_1G                /* VPN2エントリのアドレスサイズ */
	add  a7, a7, t2                          /* 仮想アドレスを更新           */
	
	add  t1, t1, a3 /* 次のVPN1テーブルのアドレスを算出  */
	add  t0, t0, 1  /* ループインデクスを加算            */
	blt  t0, a2, vpn2_setup_loop       /* 最終アドレスでなければ次のエントリを設定 */

       /*2MiBのページサイズでUARTをマップするためのVPN1テーブルを生成する
	*
	* レジスタ使用用途 (boot.Sでa0はhartid用に使用しているので使わない)
	*  a1 ページテーブルエントリの属性設定値 (pteの属性値)
	*  a2 ページマップサイズ
	*  a3 マップ対象物理アドレス
	*  a4 上位マップ側マップ対象物理アドレス
	*  a5 ページテーブルエントリサイズ
	*  a6 一時レジスタ
	*  a7 一時レジスタ		
	*/
	li a1, RV64_PTE_V|RV64_PTE_R|RV64_PTE_W|RV64_KERN_PTE_FLAGS /* フラグ値      */
	li a2, HAL_PAGE_SIZE_2M  /* ページサイズ               */
	li a3, RV64_UART0_PADDR  /* UART物理アドレス           */
	li a4, HAL_KERN_IO_BASE  /* 上位マップオフセット       */
	add a4, a4, a3           /* 上位マップUART物理アドレス */
	li a5, RV64_PGTBL_ENTRY_SIZE
	
	srli a6, a3, RV64_PGTBL_VPN1_INDEX_SHIFT /* 仮想アドレスからVPN1インデクスを算出 */
	and  a6, a6, RV64_PGTBL_VPN1_INDEX_MASK  /* VPN1インデクスをマスク               */
	mul  a6, a6, a5                          /* VPN1オフセットを設定                 */
	la   a7, pre_vpn1_uartmap                /* VPN1テーブルのアドレスをロード       */
	add  a7, a7, a6                          /* VPN1テーブルのPTEアドレス算出        */
	
	srli a6, a3, HAL_PAGE_SHIFT              /* ページフレーム番号算出      */
	slli a6, a6, RV64_PTE_PA_SHIFT           /* PTE内ページフレーム番号算出 */
	or   a6, a6, a1                          /* PTEフラグ値を設定           */
	sd   a6, (a7)                            /* ページテーブルに格納        */

       /*2MiBのページサイズでUARTをマップするためにVPN2テーブルを設定する
	* レジスタ使用用途 (boot.Sでa0はhartid用に使用しているので使わない)
	*  a1 ページテーブルエントリの属性設定値 (pteの属性値)
	*  a2 ページフレーム番号のマスク
	*  a3 上位アドレス側の仮想アドレスへのオフセット
	*  a4 下位アドレス側の仮想アドレス
	*  a5 VPN1ページテーブルエントリのアドレス
	*  a6 VPN1ページテーブルエントリサイズ
	*  t0 一時レジスタ
	*  t1 一時レジスタ
	*  t2 一時レジスタ
	*/
	li a1, RV64_PTE_V|RV64_KERN_PTE_FLAGS /* ページテーブル用フラグ値           */
	li a2, RV64_PPN_MASK                  /* ページフレーム番号マスク           */
	li a3, HAL_KERN_IO_BASE               /* カーネル仮想アドレス               */
	li a4, RV64_UART0_PADDR               /* UART物理アドレス                   */
	la a5, pre_vpn1_uartmap               /* VPN1ページテーブル                 */
	li a6, RV64_PGTBL_ENTRY_SIZE          /* ページテーブルエントリサイズ       */
	/* VPN1テーブルへのPTE値を算出
	 */
	srli t0, a5, HAL_PAGE_SHIFT        /* VPN1テーブルのページフレーム番号算出 */
	and  t0, t0, a2                    /* ページ番号をマスク                   */
	slli t0, t0, RV64_PTE_PA_SHIFT     /* PTE内ページフレーム番号算出          */
	or   t0, t0, a1                    /* PTEフラグ値を設定                    */
	
	/* 下位アドレス側のテーブル設定
	 */
	srli t1, a4, RV64_PGTBL_VPN2_INDEX_SHIFT /* 仮想アドレスからVPN2インデクスを算出 */
	and  t1, t1, RV64_PGTBL_VPN2_INDEX_MASK  /* VPN2インデクスをマスク               */
	mul  t1, t1, a6                          /* VPN2オフセットを設定                 */
	la   t2, pre_vpn2                        /* VPN2テーブルのアドレスをロード       */
	add  t2, t2, t1                          /* VPN2テーブルのPTEアドレス算出        */
	sd   t0, (t2)                            /* PTEを更新                            */

#if defined(CONFIG_UPPERHALF_KERNEL)
	/* 上位アドレス側のテーブル設定
	 */
	add  t1, a4, a3                          /* 上位仮想アドレスを算出               */
	srli t1, t1, RV64_PGTBL_VPN2_INDEX_SHIFT /* 仮想アドレスからVPN2インデクスを算出 */
	and  t1, t1, RV64_PGTBL_VPN2_INDEX_MASK  /* VPN2インデクスをマスク               */
	mul  t1, t1, a6                          /* VPN2オフセットを設定                 */
	la   t2, pre_vpn2                        /* VPN2テーブルのアドレスをロード       */
	add  t2, t2, t1                          /* VPN2テーブルのPTEアドレス算出        */
	sd   t0, (t2)                            /* PTEを更新                            */
#endif  /*  CONFIG_UPPERHALF_KERNEL  */

	ld s0, 0(sp)     /*  s0レジスタを復元する                            */
	ld ra, 8(sp)     /*  raレジスタを復元する                            */
	addi sp,sp,16    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */

#if defined(CONFIG_HAL)
ASMMAC_BOOT_DATA_SECTION
#else
ASMMAC_DATA_SECTION
#endif  /* CONFIG_HAL */	

ASMMAC_EXTERN_SYMBOL(pre_vpn2)
ASMMAC_EXTERN_SYMBOL(pre_vpn1)

ASMMAC_DECLARE_NAME(pre_vpn2)
ASMMAC_DECLARE_NAME(pre_vpn1)

ASMMAC_BALIGN(HAL_PAGE_SIZE)
pre_vpn2:
	ASMMAC_FILL(HAL_PAGE_SIZE/RV64_PGTBL_ENTRY_SIZE, RV64_PGTBL_ENTRY_SIZE, 0)

ASMMAC_BALIGN(HAL_PAGE_SIZE)
pre_vpn1:
pre_vpn1_page0:
	ASMMAC_FILL(HAL_PAGE_SIZE/RV64_PGTBL_ENTRY_SIZE, RV64_PGTBL_ENTRY_SIZE, 0)

ASMMAC_BALIGN(HAL_PAGE_SIZE)
pre_vpn1_page1:
	ASMMAC_FILL(HAL_PAGE_SIZE/RV64_PGTBL_ENTRY_SIZE, RV64_PGTBL_ENTRY_SIZE, 0)

ASMMAC_BALIGN(HAL_PAGE_SIZE)
pre_vpn1_page2:
	ASMMAC_FILL(HAL_PAGE_SIZE/RV64_PGTBL_ENTRY_SIZE, RV64_PGTBL_ENTRY_SIZE, 0)

ASMMAC_BALIGN(HAL_PAGE_SIZE)
pre_vpn1_page3:
	ASMMAC_FILL(HAL_PAGE_SIZE/RV64_PGTBL_ENTRY_SIZE, RV64_PGTBL_ENTRY_SIZE, 0)

ASMMAC_BALIGN(HAL_PAGE_SIZE)
pre_vpn1_uartmap:
	ASMMAC_FILL(HAL_PAGE_SIZE/RV64_PGTBL_ENTRY_SIZE, RV64_PGTBL_ENTRY_SIZE, 0)
	
