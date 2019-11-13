/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  C type relevant definitions                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_CTYPE_H)
#define  _KLIB_CTYPE_H 

#include <klib/freestanding.h>

/**
   英小文字であることを確認する
   @param[in] c 確認対象文字を表す整数値
   @retval 真 英小文字である
   @retval 偽 英小文字でない
*/
#define islower(c) ( ( 'a' <= (c) ) && ( (c) <= 'z' ) )
/**
   英大文字であることを確認する
   @param[in] c 確認対象文字を表す整数値
   @retval 真 英大文字である
   @retval 偽 英大文字でない
*/
#define isupper(c) ( ( 'A' <= (c) ) && ( (c) <= 'Z' ) )
/**
   数字であることを確認する
   @param[in] c 確認対象文字を表す整数値
   @retval 真 数字である
   @retval 偽 数字でない
*/
#define isdigit(c) ( ( '0' <= (c) ) && ( (c) <= '9' ) )
/**
   ブランク文字(空白またはタブ)であることを確認する
   @param[in] c 確認対象文字を表す整数値
   @retval 真 ブランク文字である
   @retval 偽 ブランク文字でない
*/
#define isblank(c) ( ( (c) == ' ' ) || ( (c)  == '\t' ) )
/**
   制御文字(0x00 - 0x1f, 0x7f)であることを確認する
   @param[in] c 確認対象文字を表す整数値
   @retval 真 制御文字である
   @retval 偽 制御文字でない
*/
#define iscntrl(c) ( ( ( 0x00 <= (c) ) && ( (c)  <= 0x1f ) ) || ( (c) == 0x7f ) )
/**
    空白以外の表示可能文字(0x21-0x7e)であることを確認する
   @param[in] c 確認対象文字を表す整数値
   @retval 真 表示可能文字である
   @retval 偽 表示可能文字でない
*/
#define isgraph(c) ( ( ( 0x21 <=  (c) ) && ( (c) <= 0x7e ) ) )
/**
   空白を含む印刷可能文字(0x20-0x7e)であることを確認する
   @param[in] c 確認対象文字を表す整数値
   @retval 真 印刷可能文字である
   @retval 偽 印刷可能文字でない
*/
#define isprint(c) ( ( 0x20 <=  (c) ) && ( (c)  <= 0x7e ) )
/**
   区切り文字であることを確認する
   @param[in] c 確認対象文字を表す整数値
   @retval 真 区切り文字である
   @retval 偽 区切り文字でない
*/
#define ispunct(c) ( ( ( 0x21 <=  (c) ) && ( (c)  <= 0x2f ) ) || \
	             ( ( 0x3a <=  (c) ) && ( (c)  <= 0x40 ) ) || \
	             ( ( 0x5b <=  (c) ) && ( (c)  <= 0x60 ) ) || \
	             ( ( 0x7b <=  (c) ) && ( (c)  <= 0x7e ) ) )
/**
   標準空白類文字であることを確認する
   @param[in] c 確認対象文字を表す整数値
   @retval 真 標準空白類文字である
   @retval 偽 標準空白類文字でない
*/
#define isspace(c) ( ( (c) == '\f' ) || ( (c)  == '\n' ) || \
	             ( (c) == '\r' ) || ( (c)  == '\v' ) || \
	             isblank(c) )
/**
   16進数字であることを確認する
   @param[in] c 確認対象文字を表す整数値
   @retval 真 16進数字である
   @retval 偽 16進数字でない
*/
#define isxdigit(c) ( isdigit(c) || ( ( 'a' <= (c) ) && ( (c) <= 'f' ) ) || \
		      ( ( 'A' <= (c) ) && ( (c) <= 'F' ) ) )
/**
   アルファベット文字であることを確認する
   @param[in] c 確認対象文字を表す整数値
   @retval 真 アルファベット文字である
   @retval 偽 アルファベット文字でない
*/
#define isalpha(c) ( islower(c) || isupper(c) )
/**
   英数字であることを確認する
   @param[in] c 確認対象文字を表す整数値
   @retval 真 英数字である
   @retval 偽 英数字でない
*/
#define isalnum(c) ( isalpha(c) || isdigit(c) )
/**
   英小文字を英大文字に変換する. 英小文字以外はそのまま返却する
   @param[in] c 確認対象文字を表す整数値
   @return 変換後の英大文字, または, 変換前の文字(入力が英小文字でない場合)
*/
#define toupper(c) ( islower(c) ? ( (c) & 0x5f ) : (c) )
/**
   英大文字を英小文字に変換する. 英大文字以外はそのまま返却する
   @param[in] c 確認対象文字を表す整数値
   @return 変換後の英小文字, または, 変換前の文字(入力が英大文字でない場合)
*/
#define tolower(c) ( isupper(c) ? ( (c) | 0x20 ) : (c) )

#endif  /*  _KLIB_CTYPE_H   */
