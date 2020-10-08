/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  UART definitions                                                  */
/*                                                                    */
/**********************************************************************/
#if !defined(_DEV_UART_H)
#define _DEV_UART_H

#define UART_RHR     (0x00)   /* 受信レジスタ        */
#define UART_THR     (0x00)   /* 送信レジスタ        */
#define UART_INTR    (0x01)   /* 割り込み許可        */
#define UART_FIFO    (0x02)   /* FIFO                */
#define UART_IIR     (0x02)   /* 割り込みID          */
#define UART_LCTRL   (0x03)   /* ラインコントロール  */
#define UART_MCTRL   (0x04)   /* モデムコントロール  */
#define UART_LSR     (0x05)   /* ラインステータス    */
#define UART_MSR     (0x06)   /* モデムステータス    */

/*
 *  割り込み関連
 */
#define UART_IIR_NO_INT	(0x01)	/* 割り込み未発生 */
#define UART_IIR_IDMASK	(0x06)	/* 割り込みIDマスク */
#define UART_IIR_MSI	(0x00)	/* モデムステータス */
#define UART_IIR_THRI	(0x02)	/* 送信完了(バッファエンプティ) */
#define UART_IIR_RDI	(0x04)	/* 受信完了 */
#define UART_IIR_RLSI	(0x06)	/* 受信回線状態更新 */
#define uart_getiir(val) ( (val) & (UART_IIR_IDMASK) )
/*
 *  モデムコントロール関連
 */
#define UART_MCTL_DTR_LOW     (0x1)
#define UART_MCTL_RTS_LOW     (0x2)
#define UART_MCTL_OUT1_LOW    (0x4)
#define UART_MCTL_ENINTR      (0x8)
#define UART_MCTL_LOOPBAK     (0x10)
#define UART_MCTL_CONST       (UART_MCTL_DTR_LOW|UART_MCTL_RTS_LOW)
#define uart_mk_mctl(func)    ( (func) | UART_MCTL_CONST )

/*
 * ラインステータス
 */
#define UART_LSR_RXRDY       (0x01)
#define UART_LSR_TXHOLD      (0x20)

/*
 * Interrupt Enable Register
 */
#define UART_INTR_DIS   (0x00) /*< 割込み通知を禁止   */
#define UART_INTR_RDA   (0x01) /*< 受信割込みを有効化 */
#define UART_INTR_TXE   (0x02) /*< 送信割込みを有効化 */


#endif  /*  _DEV_UART_H  */
