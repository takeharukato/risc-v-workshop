/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  compiler relevant definitions                                     */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_COMPILER_H)
#define  _KLIB_COMPILER_H 

#if !defined(ASM_FILE)

#include <klib/freestanding.h>

/** Fall throughマクロ コンパイラの警告を抑止するために導入 */
#if defined(__cplusplus) && __cplusplus >= 201703L
#define __FALLTHROUGH [[fallthrough]]
#elif defined(__cplusplus) && defined(__clang__)
#define __FALLTHROUGH [[clang::fallthrough]]
#elif __GNUC__ >= 7
#define __FALLTHROUGH __attribute__((__fallthrough__))
#else    /*   __GNUC__ >= 7  */
#define __FALLTHROUGH do {} while (0)
#endif  /*   __cplusplus && __cplusplus >= 201703L  */

/*
 * 関数/変数属性
 */
#if !defined(__aligned)
#define __aligned(x)     __attribute__((__aligned__(x)))
#endif  /*  !__aligned  */
#if !defined(__always_inline)
#define __always_inline  inline __attribute__((__always_inline__))
#endif  /* !__always_inline  */
#if !defined(__noreturn)
#define __noreturn       __attribute__((__noreturn__))
#endif  /*  !__noreturn  */
#if !defined(__packed)
#define __packed         __attribute__((__packed__))
#endif  /*  !__packed  */
#if !defined(__unused)
#define __unused         __attribute__((__unused__))
#endif  /*  __unused  */
#if !defined(__weak)
#define __weak           __attribute__((__weak__))
#endif  /*  !__weak  */
#if !defined(__alias)
#define __alias(symbol)  __attribute__((__alias__(#symbol)))
#endif  /*  !__alias  */
#if !defined(__section)
#define __section(sec)   __attribute__((__section__(#sec)))
#endif   /*  !__section  */
#if !defined(__initilizer)
#define __initilizer     __attribute__((constructor))
#endif   /*  !__initilizer  */
#if !defined(__finalizer)
#define __finalizer      __attribute__((destructor))
#endif   /*  !__finalizer  */

#endif  /*  !ASM_FILE  */

#endif  /*  _KLIB_COMPILER_H   */
