/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs chdir operation                                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>

#include <kern/vfs-if.h>

/**
   カレントディレクトリを変更する
   @param[in] ioctx I/Oコンテキスト
   @param[in] path  移動先ディレクトリのファイルパス
   @retval  0      正常終了
   @retval -EBADF  正当なユーザファイルディスクリプタでない
   @retval -EIO    I/Oエラー
   @retval -ENOMEM メモリ不足
   @retval -ENOSYS mkdirをサポートしていない
   @retval -EROFS  読み取り専用でマウントされている
 */
int
vfs_chdir(vfs_ioctx *ioctx, char *path){


}
