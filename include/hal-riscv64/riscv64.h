/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 page operations                                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RISCV64_H)
#define  _HAL_RISCV64_H 
#include <klib/misc.h>
#include <klib/regbits.h>
/*
 * Machine Status Register(mstatus)
 * SiFive FU540-C000 Manual
 * RISC-V Privileged Architectures V1.10 Figure 3.7
 */
#define MSTATUS_SIE_BIT      (1)
#define MSTATUS_MIE_BIT      (3)
#define MSTATUS_SPIE_BIT     (5)
#define MSTATUS_MPIE_BIT     (7)
#define MSTATUS_SPP_BIT      (8)
#define MSTATUS_MPP_MIN_BIT  (11)
#define MSTATUS_MPP_MAX_BIT  (12)
#define MSTATUS_MPP_MASK (regops_calc_mask_range(MSTATUS_MPP_MIN_BIT, MSTATUS_MPP_MAX_BIT))
/** マシンモード mstatus.MPP = 11b */
#define MSTATUS_MPP_M    (regops_calc_mask_range(MSTATUS_MPP_MIN_BIT, MSTATUS_MPP_MAX_BIT))
/** スーパバイザモード mstatus.MPP = 01b */
#define MSTATUS_MPP_S    (regops_set_bit(MSTATUS_MPP_MIN_BIT))
/** ユーザモード mstatus.MPP = 00b */
#define MSTATUS_MPP_U    (regops_calc_clrmask_range(MSTATUS_MPP_MIN_BIT, MSTATUS_MPP_MAX_BIT))
/**  マシンモード割込み許可 */
#define MSTATUS_MIE      (regops_set_bit(MSTATUS_MIE_BIT))

/* Machine Interrupt Registers
 * Figure 3.11: Machine interrupt-pending register
 */
#define MIREG_USIP_BIT     (0)  /* ユーザモードソフトウエア割込み */
#define MIREG_SSIP_BIT     (1)  /* スーパーバイザモードソフトウエア割込み */
#define MIREG_MSIP_BIT     (3)  /* マシンモードソフトウエア割込み */
#define MIREG_UTIP_BIT     (4)  /* ユーザモードタイマ割込み */
#define MIREG_STIP_BIT     (5)  /* スーパーバイザモードタイマ割込み */
#define MIREG_MTIP_BIT     (7)  /* マシンモードタイマ割込み */
#define MIREG_UEIP_BIT     (8)  /* ユーザモード外部割込み */
#define MIREG_SEIP_BIT     (9)  /* スーパーバイザモード外部割込み */
#define MIREG_MEIP_BIT     (11) /* マシンモード外部割込み */

/* Machine Interrupt Pendingレジスタ
 */
#define MIP_USIP (regops_set_bit(MIREG_USIP_BIT))
#define MIP_SSIP (regops_set_bit(MIREG_SSIP_BIT))
#define MIP_MSIP (regops_set_bit(MIREG_MSIP_BIT))
#define MIP_UTIP (regops_set_bit(MIREG_UTIP_BIT))
#define MIP_STIP (regops_set_bit(MIREG_STIP_BIT))
#define MIP_MTIP (regops_set_bit(MIREG_MTIP_BIT))
#define MIP_UEIP (regops_set_bit(MIREG_UEIP_BIT))
#define MIP_SEIP (regops_set_bit(MIREG_SEIP_BIT))
#define MIP_MEIP (regops_set_bit(MIREG_MEIP_BIT))

/* Machine Interrupt Pendingマスク
 */
#define MIP_MASK (MIP_USIP | MIP_SSIP | MIP_MSIP | \
	          MIP_UTIP | MIP_STIP | MIP_MTIP | \
	          MIP_UEIP | MIP_SEIP | MIP_MEIP )

/* Machine Interrupt Enableレジスタ
 */
#define MIE_USIP (MIP_USIP)
#define MIE_SSIP (MIP_SSIP)
#define MIE_MSIP (MIP_MSIP)
#define MIE_UTIP (MIP_UTIP)
#define MIE_STIP (MIP_STIP)
#define MIE_MTIP (MIP_MTIP)
#define MIE_UEIP (MIP_UEIP)
#define MIE_SEIP (MIP_SEIP)
#define MIE_MEIP (MIP_MEIP)

/* Machine Interrupt Enableマスク
 */
#define MIE_MASK ( MIP_MASK )

/*
 * トラップ種別 (Machine cause register)
 * Table 3.6: Machine cause register (mcause) values after trap.
 */
#define MCAUSE_INTR_BIT           (63) /* 割込みビット */
#define MCAUSE_USER_SINT_BIT      (0)  /* ユーザモードソフトウエア割込み */
#define MCAUSE_SUPER_SINT_BIT     (1)  /* スーパバイザモードソフトウエア割込み */
#define MCAUSE_RESERVED1_BIT      (2)  /* 予約 */
#define MCAUSE_MACHINE_SINT_BIT   (3)  /* マシンモードソフトウエア割込み */
#define MCAUSE_USER_TIMER_BIT     (4)  /* ユーザモードタイマ割込み */
#define MCAUSE_SUPER_TIMER_BIT    (5)  /* スーパバイザモードタイマ割込み */
#define MCAUSE_RESERVED2_BIT      (6)  /* 予約 */
#define MCAUSE_MACHINE_TIMER_BIT  (7)  /* マシンモードタイマ割込み */
#define MCAUSE_USER_EXTINT_BIT    (8)  /* ユーザモード外部割込み */
#define MCAUSE_SUPER_EXTINT_BIT   (9)  /* スーパバイザモード外部割込み */
#define MCAUSE_RESERVED3_BIT      (10) /* 予約 */
#define MCAUSE_MACHINE_EXTINT_BIT (11) /* マシンモード外部割込み */

#define MCAUSE_IMISALIGN_BIT      (0)  /* 命令ミスアラインメント例外        */
#define MCAUSE_IACSFAULT_BIT      (1)  /* 命令アクセスフォルト例外          */
#define MCAUSE_ILLINST_BIT        (2)  /* 不正命令例外                      */
#define MCAUSE_BRKPOINT_BIT       (3)  /* ブレークポイント例外              */
#define MCAUSE_LDMISALIGN_BIT     (4)  /* ロードミスアラインメント例外      */
#define MCAUSE_LDACSFAULT_BIT     (5)  /* ロードアクセス例外                */
#define MCAUSE_STMISALIGN_BIT     (6)  /* ストア/AMOミスアラインメント例外  */
#define MCAUSE_STACSFAULT_BIT     (7)  /* ストア/AMOアクセス例外            */
#define MCAUSE_ENVCALL_UMODE_BIT  (8)  /* ユーザ環境呼び出し                */
#define MCAUSE_ENVCALL_SMODE_BIT  (9)  /* スーパバイザモード環境呼び出し    */
#define MCAUSE_RESERVED4_BIT      (10) /* 予約                              */
#define MCAUSE_ENVCALL_MMODE_BIT  (11) /* マシンモード環境呼び出し          */
#define MCAUSE_I_PGFAULT_BIT      (12) /* 命令ページフォルト                */
#define MCAUSE_LD_PGFAULT_BIT     (13) /* ロードページフォルト              */
#define MCAUSE_RESERVED5_BIT      (14) /* 予約                              */
#define MCAUSE_ST_PGFAULT_BIT     (15) /* ストアページフォルト              */

/*
 * macuseレジスタ割込み要因
 */
#define MCAUSE_USER_SINT      \
	( regops_set_bit(MCAUSE_INTR_BIT) | regops_set_bit(MCAUSE_USER_SINT_BIT) )
#define MCAUSE_SUPER_SINT     \
	( regops_set_bit(MCAUSE_INTR_BIT) | regops_set_bit(MCAUSE_SUPER_SINT_BIT) )	
#define MCAUSE_RESERVED1      \
	( regops_set_bit(MCAUSE_INTR_BIT) | regops_set_bit(MCAUSE_RESERVED1_BIT) )	
#define MCAUSE_MACHINE_SINT   \
	( regops_set_bit(MCAUSE_INTR_BIT) | regops_set_bit(MCAUSE_MACHINE_SINT_BIT) )	
#define MCAUSE_USER_TIMER     \
	( regops_set_bit(MCAUSE_INTR_BIT) | regops_set_bit(MCAUSE_USER_TIMER_BIT) )
#define MCAUSE_SUPER_TIMER    \
	( regops_set_bit(MCAUSE_INTR_BIT) | regops_set_bit(MCAUSE_SUPER_TIMER_BIT) )
#define MCAUSE_RESERVED2      \
	( regops_set_bit(MCAUSE_INTR_BIT) | regops_set_bit(MCAUSE_RESERVED2_BIT) )
#define MCAUSE_MACHINE_TIMER  \
	( regops_set_bit(MCAUSE_INTR_BIT) | regops_set_bit(MCAUSE_MACHINE_TIMER_BIT) )
#define MCAUSE_USER_EXTINT    \
	( regops_set_bit(MCAUSE_INTR_BIT) | regops_set_bit(MCAUSE_USER_EXTINT_BIT) )
#define MCAUSE_SUPER_EXTINT   \
	( regops_set_bit(MCAUSE_INTR_BIT) | regops_set_bit(MCAUSE_SUPER_EXTINT_BIT) )
#define MCAUSE_RESERVED3      \
	( regops_set_bit(MCAUSE_INTR_BIT) | regops_set_bit(MCAUSE_RESERVED3_BIT) )
#define MCAUSE_MACHINE_EXTINT \
	( regops_set_bit(MCAUSE_INTR_BIT) | regops_set_bit(MCAUSE_MACHINE_EXTINT_BIT) )

/*
 * macuseレジスタ例外要因
 */
#define MCAUSE_IMISALIGN     (regops_set_bit(MCAUSE_IMISALIGN_BIT))
#define MCAUSE_IACSFAULT     (regops_set_bit(MCAUSE_IACSFAULT_BIT))
#define MCAUSE_ILLINST       (regops_set_bit(MCAUSE_ILLINST_BIT))
#define MCAUSE_BRKPOINT      (regops_set_bit(MCAUSE_BRKPOINT_BIT))
#define MCAUSE_LDMISALIGN    (regops_set_bit(MCAUSE_LDMISALIGN_BIT))
#define MCAUSE_LDACSFAULT    (regops_set_bit(MCAUSE_LDACSFAULT_BIT))
#define MCAUSE_STMISALIGN    (regops_set_bit(MCAUSE_STMISALIGN_BIT))
#define MCAUSE_STACSFAULT    (regops_set_bit(MCAUSE_STACSFAULT_BIT))
#define MCAUSE_ENVCALL_UMODE (regops_set_bit(MCAUSE_ENVCALL_UMODE_BIT))
#define MCAUSE_ENVCALL_SMODE (regops_set_bit(MCAUSE_ENVCALL_SMODE_BIT))
#define MCAUSE_ENVCALL_MMODE (regops_set_bit(MCAUSE_ENVCALL_MMODE_BIT))
#define MCAUSE_I_PGFAULT     (regops_set_bit(MCAUSE_I_PGFAULT_BIT))
#define MCAUSE_LD_PGFAULT    (regops_set_bit(MCAUSE_LD_PGFAULT_BIT))
#define MCAUSE_ST_PGFAULT    (regops_set_bit(MCAUSE_ST_PGFAULT_BIT))

/* 例外回送マスク */
#define MEDELEG_MASK					\
	  ( MCAUSE_IMISALIGN     |			\
	    MCAUSE_IACSFAULT     |			\
	    MCAUSE_ILLINST       |			\
	    MCAUSE_BRKPOINT      |			\
	    MCAUSE_LDMISALIGN    |			\
	    MCAUSE_LDACSFAULT    |			\
	    MCAUSE_STMISALIGN    |			\
	    MCAUSE_STACSFAULT    |			\
	    MCAUSE_ENVCALL_UMODE |			\
	    MCAUSE_ENVCALL_SMODE |			\
	    MCAUSE_ENVCALL_MMODE |			\
	    MCAUSE_I_PGFAULT     |			\
	    MCAUSE_LD_PGFAULT    |			\
	    MCAUSE_ST_PGFAULT )

/* 割込み回送マスク */
#define MIDELEG_MASK (MIE_MASK)

#endif  /*  _HAL_RISCV64_H  */
