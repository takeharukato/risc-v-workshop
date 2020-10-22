/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Assembler macro definitions                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_ASM_MACROS_H)
#define _KLIB_ASM_MACROS_H

#if defined(ASM_FILE)
/**
   バイトアラインメントを設定する
   @param[in] _byte_align 設定するアラインメント(単位:バイト)
 */
#define ASMMAC_BALIGN(_byte_align)     .balign _byte_align
/**
   リピート命令開始
   @param[in] _count 繰返し回数
 */
#define ASMMAC_REPEAT(_count)          .rept   _count
/**
   リピート命令終了
 */
#define ASMMAC_END_REPEAT()            .endr
/**
   指定バイトをフィルする
   @param[in] _count フィルする回数
   @param[in] _size  1回あたりのサイズ(単位:バイト)
   @param[in] _val   フィルする値
 */
#define ASMMAC_FILL(_count,_size,_val)      .fill   _count,_size,_val
/**
   プログラムオリジン(位置カウンタ)を指定された位置に設定する
   @param[in] _offset プログラムオリジン (単位: バイト)
 */
#define ASMMAC_SET_ORIGIN(_offset)     .org    _offset
/**
   外部シンボル参照を宣言する
   @param[in] _name ラベル名
 */
#define ASMMAC_EXTERN_SYMBOL(_name)     .extern _name

/**
   グローバルシンボルを宣言する
 */
#define ASMMAC_DECLARE_NAME(_name)		\
	.global _name;

/**
   テキスト領域内関数を定義する
   @param[in] _name 関数名
 */
#define ASMMAC_FUNCTION(_name)					\
	.global _name;					\
	.section .text, "ax", @progbits;		\
_name:

/**
   テキストセクションを開始する
 */
#define ASMMAC_TEXT_SECTION		\
	.section .text, "ax", @progbits;\

/**
   データセクションを開始する
 */
#define ASMMAC_DATA_SECTION		\
	.section .data, "aw", @progbits;\

/**
   ブート領域内関数を定義する
   @param[in] _name 関数名
 */
#define ASMMAC_BOOT_FUNCTION(_name)				\
	.global _name;					\
	.section .boot.text, "ax", @progbits;		\
_name:

/**
   ブートテキストセクションを開始する
 */
#define ASMMAC_BOOT_TEXT_SECTION		\
	.section .boot.text, "ax", @progbits;\
/**
   ブートデータセクションを開始する
 */
#define ASMMAC_BOOT_DATA_SECTION		\
	.section .boot.data, "aw", @progbits;	\

#else /* !ASM_FILE */
#define ASMMAC_BALIGN(_byte_align)
#define ASMMAC_REPEAT(_count)
#define ASMMAC_END_REPEAT()
#define ASMMAC_FILL(_count)
#define ASMMAC_SET_ORIGIN(_offset)
#define ASMMAC_EXTERN_SYMBOL(_name)
#define ASMMAC_DECLARE_NAME(_name)
#define ASMMAC_FUNCTION(_name)
#define ASMMAC_TEXT_SECTION
#define ASMMAC_DATA_SECTION
#define ASMMAC_BOOT_FUNCTION(_name)
#define ASMMAC_BOOT_TEXT_SECTION
#define ASMMAC_BOOT_DATA_SECTION
#endif  /* ASM_FILE */
#endif  /* _KLIB__ASM_MACROS_H */
