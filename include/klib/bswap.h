/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Byte swap operations                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_KLIB_BSWAP_H)
#define  _KLIB_KLIB_BSWAP_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
uint64_t __bswap64(uint64_t _x);
uint32_t __bswap32(uint32_t _x);
uint16_t __bswap16(uint16_t _x);
#endif  /* !ASM_FILE */
#endif  /* KLIB_KLIB_BSWAP_H */
