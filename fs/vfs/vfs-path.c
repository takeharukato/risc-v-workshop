/* -*- mode: asm; coding:utf-8 -*- */
/************************************************************************/
/*  OS kernel sample                                                    */
/*  Copyright 2019 Takeharu KATO                                        */
/*                                                                      */
/*  VFS path operation                                                  */
/*                                                                      */
/************************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>

/**
   ファイルの終端が/だったら/.に変換する
 */
int
new_path(const char *path, char **copy){
}

/**
   パスを解放する
 */
void
free_path(char *p){

    if ( p != NULL )
	    kfree(p);
    
}
