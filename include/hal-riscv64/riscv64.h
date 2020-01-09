/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 definitions                                             */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RISCV64_H)
#define  _HAL_RISCV64_H 
#include <klib/misc.h>
#include <klib/regbits.h>
/*
 * Machine Status Register(mstatus) (partial)
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

/*Supervisor Status Register, sstatus
 */
#define SSTATUS_SD_BIT   (63)  /* Some dirty */
#define SSTATUS_UXL_BIT  (32)  /* XLEN for U-mode */
#define SSTATUS_MXR_BIT  (19)  /* Make eXecutable Readable */
#define SSTATUS_SUM_BIT  (18)  /* Supervisor User Memory access */
#define SSTATUS_XS_BIT   (15)  /* User-mode extensions */
#define SSTATUS_FS_BIT   (13)  /* Current state of the floating-point unit */
#define SSTATUS_SPP_BIT   (8)  /* Previous mode, 1=Supervisor, 0=User */
#define SSTATUS_SPIE_BIT  (5)  /* Supervisor Previous Interrupt Enable */
#define SSTATUS_UPIE_BIT  (4)  /* User Previous Interrupt Enable */
#define SSTATUS_SIE_BIT   (1)  /* Supervisor Interrupt Enable */
#define SSTATUS_UIE_BIT   (0)  /* User Interrupt Enable */

#define SSTATUS_SD         (regops_set_bit(SSTATUS_SD_BIT))
#define SSTATUS_UXL        (regops_calc_mask_range(SSTATUS_UXL_BIT,SSTATUS_UXL_BIT+1))
#define SSTATUS_MXR        (regops_set_bit(SSTATUS_MXR_BIT))
#define SSTATUS_SUM        (regops_set_bit(SSTATUS_SUM_BIT))
#define SSTATUS_XS         (regops_calc_mask_range(SSTATUS_XS_BIT,SSTATUS_XS_BIT+1))
#define SSTATUS_FS         (regops_calc_mask_range(SSTATUS_FS_BIT,SSTATUS_FS_BIT+1))
#define SSTATUS_SPP        (regops_set_bit(SSTATUS_SPP_BIT))
#define SSTATUS_SPIE       (regops_set_bit(SSTATUS_SPIE_BIT))
#define SSTATUS_UPIE       (regops_set_bit(SSTATUS_UPIE_BIT))
#define SSTATUS_SIE        (regops_set_bit(SSTATUS_SIE_BIT))
#define SSTATUS_UIE        (regops_set_bit(SSTATUS_UIE_BIT))

/* Machine Interrupt Registers
 * RISC-V Privileged Architectures V1.10 Figure 3.11: Machine interrupt-pending register
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
#define MIE_USIE (MIP_USIP)
#define MIE_SSIE (MIP_SSIP)
#define MIE_MSIE (MIP_MSIP)
#define MIE_UTIE (MIP_UTIP)
#define MIE_STIE (MIP_STIP)
#define MIE_MTIE (MIP_MTIP)
#define MIE_UEIE (MIP_UEIP)
#define MIE_SEIE (MIP_SEIP)
#define MIE_MEIE (MIP_MEIP)

/* Machine Interrupt Enableマスク
 */
#define MIE_MASK ( MIP_MASK )

/* Supervisor Interrupt Registers
 * RISC-V Privileged Architectures V1.10  Figure 3.11: Machine interrupt-pending register
 */
#define SIREG_USIP_BIT     (0)  /* ユーザモードソフトウエア割込み */
#define SIREG_SSIP_BIT     (1)  /* スーパーバイザモードソフトウエア割込み */
#define SIREG_MSIP_BIT     (3)  /* マシンモードソフトウエア割込み */
#define SIREG_UTIP_BIT     (4)  /* ユーザモードタイマ割込み */
#define SIREG_STIP_BIT     (5)  /* スーパーバイザモードタイマ割込み */
#define SIREG_MTIP_BIT     (7)  /* マシンモードタイマ割込み */
#define SIREG_UEIP_BIT     (8)  /* ユーザモード外部割込み */
#define SIREG_SEIP_BIT     (9)  /* スーパーバイザモード外部割込み */
#define SIREG_MEIP_BIT     (11) /* マシンモード外部割込み */

/* Supervisor Interrupt Pendingレジスタ
 */
#define SIP_USIP (regops_set_bit(SIREG_USIP_BIT))
#define SIP_SSIP (regops_set_bit(SIREG_SSIP_BIT))
#define SIP_MSIP (regops_set_bit(SIREG_MSIP_BIT))
#define SIP_UTIP (regops_set_bit(SIREG_UTIP_BIT))
#define SIP_STIP (regops_set_bit(SIREG_STIP_BIT))
#define SIP_MTIP (regops_set_bit(SIREG_MTIP_BIT))
#define SIP_UEIP (regops_set_bit(SIREG_UEIP_BIT))
#define SIP_SEIP (regops_set_bit(SIREG_SEIP_BIT))
#define SIP_MEIP (regops_set_bit(SIREG_MEIP_BIT))

/* Supervisor Interrupt Pendingマスク
 */
#define SIP_MASK (SIP_USIP | SIP_SSIP | SIP_MSIP | \
	          SIP_UTIP | SIP_STIP | SIP_MTIP | \
	          SIP_UEIP | SIP_SEIP | SIP_MEIP )

/* Supervisor Interrupt Enableレジスタ
 */
#define SIE_USIE (SIP_USIP)
#define SIE_SSIE (SIP_SSIP)
#define SIE_MSIE (SIP_MSIP)
#define SIE_UTIE (SIP_UTIP)
#define SIE_STIE (SIP_STIP)
#define SIE_MTIE (SIP_MTIP)
#define SIE_UEIE (SIP_UEIP)
#define SIE_SEIE (SIP_SEIP)
#define SIE_MEIE (SIP_MEIP)

/* Supervisor Interrupt Enableマスク
 */
#define SIE_MASK ( SIP_MASK )

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
#define MCAUSE_INTR          (regops_set_bit(MCAUSE_INTR_BIT))
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

/*
 * sacuseレジスタ例外要因
 */
#define SCAUSE_INTR          (regops_set_bit(MCAUSE_INTR_BIT))
#define SCAUSE_IMISALIGN     (regops_set_bit(MCAUSE_IMISALIGN_BIT))
#define SCAUSE_IACSFAULT     (regops_set_bit(MCAUSE_IACSFAULT_BIT))
#define SCAUSE_ILLINST       (regops_set_bit(MCAUSE_ILLINST_BIT))
#define SCAUSE_BRKPOINT      (regops_set_bit(MCAUSE_BRKPOINT_BIT))
#define SCAUSE_LDMISALIGN    (regops_set_bit(MCAUSE_LDMISALIGN_BIT))
#define SCAUSE_LDACSFAULT    (regops_set_bit(MCAUSE_LDACSFAULT_BIT))
#define SCAUSE_STMISALIGN    (regops_set_bit(MCAUSE_STMISALIGN_BIT))
#define SCAUSE_STACSFAULT    (regops_set_bit(MCAUSE_STACSFAULT_BIT))
#define SCAUSE_ENVCALL_UMODE (regops_set_bit(MCAUSE_ENVCALL_UMODE_BIT))
#define SCAUSE_ENVCALL_SMODE (regops_set_bit(MCAUSE_ENVCALL_SMODE_BIT))
#define SCAUSE_ENVCALL_MMODE (regops_set_bit(MCAUSE_ENVCALL_MMODE_BIT))
#define SCAUSE_I_PGFAULT     (regops_set_bit(MCAUSE_I_PGFAULT_BIT))
#define SCAUSE_LD_PGFAULT    (regops_set_bit(MCAUSE_LD_PGFAULT_BIT))
#define SCAUSE_ST_PGFAULT    (regops_set_bit(MCAUSE_ST_PGFAULT_BIT))

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
	    MCAUSE_ENVCALL_MMODE |			\
	    MCAUSE_I_PGFAULT     |			\
	    MCAUSE_LD_PGFAULT    |			\
	    MCAUSE_ST_PGFAULT )

/* 割込み回送マスク */
#define MIDELEG_MASK (MIE_MASK)
/* [m|h|s]counterenの設定値 */
#define COUNTEREN_VAL                    (~(UINT32_C(0)))

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/kern-types.h>

uint64_t rv64_read_sstatus(void);
uint64_t rv64_write_sstatus(uint64_t _new);
uint64_t rv64_read_mie(void);
uint64_t rv64_write_mie(uint64_t _new);
uint64_t rv64_read_sie(void);
uint64_t rv64_write_sie(uint64_t _new);
uint64_t rv64_read_sip(void);
uint64_t rv64_write_sip(uint64_t _new);
uint64_t rv64_read_tp(void);
uint64_t rv64_write_tp(uint64_t _new);
uint64_t rv64_read_time(void);
uint64_t rv64_read_cycle(void);
uint64_t rv64_read_instret(void);
uint64_t rv64_read_scounteren(void);
void rv64_write_satp(uint64_t _stap);
void rv64_flush_tlb_local(void);
void rv64_flush_tlb_vaddr(uint64_t _vaddr, uint64_t _asid);
void rv64_fence_i(void);
#endif  /* !ASM_FILE */
#endif  /*  _HAL_RISCV64_H  */
