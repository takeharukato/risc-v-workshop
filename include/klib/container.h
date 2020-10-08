/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Container operations                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_CONTAINER_H)
#define  _KLIB_CONTAINER_H

#include <klib/freestanding.h>

/**
   構造体に埋め込まれたデータ構造のオフセットアドレスを算出する
    @param[in] t データ構造を埋め込んでいる構造体の型
    @param[in] m データ構造のメンバの名前
 */
#define offset_of(t, m)			\
	( (void *)( &( ( (t *)(0) )->m ) ) )

/**
   構造体に埋め込まれたデータ構造（リストなど)のアドレスから構造体へのポインタを得る
    @param[in] p データ構造のアドレス
    @param[in] t データ構造を埋め込んでいる構造体の型
    @param[in] m データ構造を埋め込んでいる構造体中のデータ構造体メンバの名前
 */
#define container_of(p, t, m)			\
	( (t *)( ( (void *)(p) ) - offset_of(t, m) ) )

#endif  /*  _KLIB_CONTAINER_H   */
