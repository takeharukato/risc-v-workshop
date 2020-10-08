/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V debug console routines                                     */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <dev/uart.h>
#include <kern/irq-if.h>
#include <hal/rv64-platform.h>
#include <hal/hal-traps.h>

/**
   UARTレジスタを参照する
   @param[in] reg レジスタオフセットアドレス
 */
#define dbg_uart_reg(reg) ((volatile uint8_t *)(RV64_UART0 + (reg)))

/**
   UARTレジスタを読み込む
   @param[in] reg 読み込み先レジスタオフセットアドレス
 */
#define dbg_read_uart_reg(reg) (*(dbg_uart_reg(reg)))

/**
   UARTレジスタに書き込む
   @param[in] reg 書き込み先レジスタオフセットアドレス
   @param[in] v   書き込む値
 */
#define dbg_write_uart_reg(reg, v) (*(dbg_uart_reg(reg)) = (v))

/**
   UART割り込みハンドラ
   @param[in] irq     割込み番号
   @param[in] ctx     割込みコンテキスト
   @param[in] private ハンドラプライベート情報
   @retval IRQ_HANDLED 割込みを処理した
 */
static int
uart_irq_handler(irq_no irq, trap_context *ctx, void *private){
	int ch;

	/* 受信可能になるのを待ち合わせる */
	while((dbg_read_uart_reg(UART_LSR) & UART_LSR_RXRDY) == 0);

	ch = dbg_read_uart_reg(UART_RHR); /* 1文字読み込む */
	hal_kconsole_putchar(ch);
	if ( ch == '\r' )
		hal_kconsole_putchar('\n');
	else if ( ch == '\b' ) {

		hal_kconsole_putchar(' ');
		hal_kconsole_putchar('\b');
	}

	return IRQ_HANDLED;
}

/**
   UART受信割り込みを有効化する
 */
void
uart_rxintr_enable(void){
	int rc;

	rc = irq_register_handler(RV64_UART0_IRQ, IRQ_ATTR_NESTABLE|IRQ_ATTR_EDGE, 1,
	    uart_irq_handler, NULL);
	kassert( rc == 0 );

	/* 割込み通知を有効化 */
	dbg_write_uart_reg(UART_INTR, UART_INTR_RDA);
}

/**
   デバッグ用コンソールを初期化する
 */
void
hal_dbg_console_init(void){

	/* 割込み通知を無効化 */
	dbg_write_uart_reg(UART_INTR, UART_INTR_DIS);

	/* baud rateの設定 */
	dbg_write_uart_reg(UART_LCTRL, 0x80);

	/* 38.4K baud rateに設定
	 */
	dbg_write_uart_reg(0, 0x03);  /* LSB */
	dbg_write_uart_reg(1, 0x00);  /* MSB */

	/* 8bit パリティなしに設定し回線速度設定モードを抜ける
	 */
	dbg_write_uart_reg(UART_LCTRL, 0x03);

	/* FIFOを有効化する */
	dbg_write_uart_reg(UART_FIFO, 0x07);
}

/**
   コンソールに1文字出力する
   @param[in] ch 出力する文字
 */
void
hal_kconsole_putchar(int ch){

	/* 送信可能になるのを待ち合わせる */
	while((dbg_read_uart_reg(UART_LSR) & UART_LSR_TXHOLD) == 0);

	dbg_write_uart_reg(UART_THR, ch); /* 1文字書き込む */
}
