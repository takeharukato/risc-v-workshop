/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Architechture depending miscellaneous macros and functions        */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_HAL_MISC_H)
#define  _HAL_HAL_MISC_H 

#if !defined(ASM_FILE)
/**
   32bit符号付き整数定数
   @param[in] _v 整数値
 */
#if !defined(INT32_C)
#define INT32_C(_v) ( _v )
#endif  /*  INT32_C  */
/**
   64bit符号付き整数定数
   @param[in] _v 整数値
 */
#if !defined(INT64_C)
#define INT64_C(_v) ( _v ## LL )
#endif  /*  INT64_C  */

/**
   32bit符号なし整数定数
   @param[in] _v 整数値
 */
#if !defined(UINT32_C)
#define UINT32_C(_v) ( _v ## U )
#endif  /*  UINT32_C  */

/**
   64bit符号なし整数定数
   @param[in] _v 整数値
 */
#if !defined(UINT64_C)
#define UINT64_C(_v) ( _v ## ULL )
#endif  /*  UINT64_C  */

#else  /* ASM_FILE */
/**
   32bit符号付き整数定数 (アセンブラファイル用)
   @param[in] _v 整数値
 */
#if defined(INT32_C)
#undef INT32_C
#endif  /*  INT32_C  */
#define INT32_C(_v) ( _v )


/**
   64bit符号付き整数定数 (アセンブラファイル用)
   @param[in] _v 整数値
 */
#if defined(INT64_C)
#undef INT64_C
#endif  /*  INT64_C  */
#define INT64_C(_v) ( _v )


/**
   32bit符号なし整数定数 (アセンブラファイル用)
   @param[in] _v 整数値
 */
#if defined(UINT32_C)
#undef UINT32_C
#endif  /*  UINT32_C  */
#define UINT32_C(_v) ( _v )


/**
   64bit符号なし整数定数 (アセンブラファイル用)
   @param[in] _v 整数値
 */
#if defined(UINT64_C)
#undef UINT64_C
#endif  /*  UINT64_C  */
#define UINT64_C(_v) ( _v )


#endif  /* !ASM_FILE */
#endif  /*  _HAL_HAL_MISC_H  */
