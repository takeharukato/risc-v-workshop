/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Process operations                                                */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>
#include <kern/kern-if.h>
#include <kern/sched-if.h>
#include <kern/proc-if.h>
#include <kern/thr-if.h>

static kmem_cache proc_cache; /**< プロセス管理情報のSLABキャッシュ */
static proc_db g_procdb = __PROCDB_INITIALIZER(&g_procdb); /**< プロセス管理ツリー */
static proc   *kern_proc;    /**< カーネルプロセスのプロセス管理情報 */

/** プロセス管理木
 */
static int _procdb_cmp(struct _proc *_key, struct _proc *_ent);
RB_GENERATE_STATIC(_procdb_tree, _proc, ent, _procdb_cmp);

/** 
    プロセス管理データベースエントリ比較関数
    @param[in] key 比較対象領域1
    @param[in] ent データベース内の各エントリ
    @retval 正  keyのpidが entのpidより前にある
    @retval 負  keyのpidが entのpidより後にある
    @retval 0   keyのpidが entのpidに等しい
 */
static int 
_procdb_cmp(struct _proc *key, struct _proc *ent){
	
	if ( key->id < ent->id )
		return 1;

	if ( key->id > ent->id )
		return -1;

	return 0;	
}

/**
   プロセス管理情報を割り当てる(共通関数)
   @param[out] procp プロセス管理情報返却アドレス
   @retval     0     正常終了
   @retval    -ENOMEM メモリ不足
 */
static int
allocate_process_common(proc **procp){
	int            rc;
	int             i;
	proc_segment *seg;
	proc    *new_proc;

	/* プロセス管理情報を割り当てる
	 */
	rc = slab_kmem_cache_alloc(&proc_cache, KMALLOC_NORMAL, (void **)&new_proc);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto error_out;  /* メモリ不足 */
	}

	spinlock_init(&new_proc->lock); /* プロセス管理情報のロックを初期化  */
	/* 参照カウンタを初期化(マスタースレッドからの参照分) */
	refcnt_init(&new_proc->refs); 
	queue_init(&new_proc->thrque);  /* スレッドキューの初期化      */
	new_proc->id = PROC_KERN_PID;   /* PIDをカーネル空間IDに設定   */

	/** セグメントの初期化
	 */
	for( i = 0; PROC_SEG_NR > i; ++i) {

		seg = &new_proc->segments[i];         /* セグメント情報参照     */
		seg->start = 0;             /* セグメント開始アドレス */
		seg->end = 0;               /* セグメント終了アドレス */
		seg->prot = VM_PROT_NONE;   /* 保護属性               */
		seg->flags = VM_FLAGS_NONE; /* マップ属性             */
	}
	new_proc->name[0] = '\0';  /* プロセス名を空文字列に初期化する */

	*procp = new_proc;  /* プロセス管理情報を返却 */

	return 0;

error_out:
	return rc;
}

/**
   カーネルプロセス用のプロセス管理情報を割り当てる
 */
static void
init_kernel_process(void){
	int rc;

	/* プロセス管理情報を割り当てる
	 */
	rc = allocate_process_common(&kern_proc);
	kassert( rc == 0 );

	kern_proc->pgt = hal_refer_kernel_pagetable(); /* カーネルページテーブルを設定 */
	kern_proc->id = PROC_KERN_PID;                 /* プロセスIDを0設定            */

	return ;
}

/**
   ユーザプロセスのセグメントを解放する(内部関数)
   @param[in]  p          プロセス管理情報
   @param[in]  start      アンマップする仮想空間中の開始アドレス
   @param[in]  end        アンマップする仮想空間中の終了アドレス
   @param[in]  flags      ページ割り当て要否の判断に使用するマップ属性
 */
static void
release_process_segment(proc *p, vm_vaddr start, vm_vaddr end, vm_flags flags){
	int                rc;
	vm_vaddr     rm_vaddr;
	vm_paddr     rm_paddr;
	vm_prot       rm_prot;
	vm_flags     rm_flags;
	vm_size     rm_pgsize;
	vm_vaddr    sta_vaddr;
	vm_vaddr    end_vaddr;

	/* アドレスをページ境界にそろえる
	 */
	sta_vaddr = PAGE_TRUNCATE(start); /* 開始仮想アドレス */
	end_vaddr = PAGE_ROUNDUP(end);    /* 終了仮想アドレス */

	/* 領域に割り当てられたマッピングを解放する
	 */
	for( rm_vaddr = sta_vaddr; end_vaddr > rm_vaddr; ) {

		/* 領域のマップ情報を得る
		 */
		rc = hal_pgtbl_extract(p->pgt, rm_vaddr, &rm_paddr, &rm_prot, &rm_flags,
		    &rm_pgsize);
		if ( rc == 0 ) { /* メモリがマップされている場合はマッピングを解放する */
			
			rc = vm_unmap(p->pgt, rm_vaddr, flags, rm_pgsize);
			kassert( rc == 0 );
		}

		rm_vaddr += rm_pgsize; /* 次のページ */
	}

	return ;
}

/**
   ユーザプロセス管理情報を解放する(内部関数)
   @param[in] p プロセス管理情報
 */
static void
free_user_process(proc *p){
	int             i;
	proc_segment *seg;

	kassert( p != kern_proc );  /* カーネルプロセスでないことを確認 */

	/** プロセスのセグメントを解放する
	 */
	for(i = PROC_TEXT_SEG; PROC_SEG_NR > i; ++i) {

		seg = &p->segments[i];         /* セグメント情報参照     */
		/* 領域を開放 */
		release_process_segment(p, seg->start, seg->end, seg->flags);
	}
	
	pgtbl_free_user_pgtbl(p->pgt);         /* ページテーブルを解放 */
	p->id = HAL_PGTBL_KERNEL_ASID;  /* カーネル空間IDに設定 */

	slab_kmem_cache_free(p); /* プロセス情報を解放する */	

	return ;
}

/**
   引数領域の大きさを算出する (内部関数)
   @param[in]  src         引数を読込むプロセス
   @param[in]  argv        引数配列のアドレス
   @param[in]  environment 環境変数配列のアドレス
   @param[out] argv_nrp    引数の個数を返却する領域
   @param[out] env_nrp     環境変数の個数を返却する領域
   @param[out] sizp        引数領域長返却領域
   @retval     0           正常終了
   @retval    -EFAULT      メモリアクセス不可
 */
static int
calc_argument_areasize(proc *src, const char *argv[], const char *environment[], 
    int *argv_nrp, int *env_nrp, size_t *sizp){
	int           rc;
	int            i;
	size_t       len;
	size_t array_len;
	size_t  argv_len;
	size_t   env_len;
	int      nr_args;
	int      nr_envs;

	/* argc, argv, environmentを配置するために必要な領域長を算出する
	 */
	for(i = 0, argv_len = 1; argv[i] != NULL; ++i) { /* NULLターミネート分を足す */

		len = vm_strlen(src->pgt, argv[i]);
		if ( len == 0 ) {

			rc = -EFAULT;  /* アクセスできなかった */
			goto error_out;
		}
		argv_len +=  len + 1;  /* 文字列長+NULLターミネート */
	}
	nr_args = i + 1;  /* 引数の数 + NULL文字列 */

	for(i = 0, env_len = 1; environment[i] != NULL; ++i) { /* NULLターミネート分を足す */

		len = vm_strlen(src->pgt, environment[i]);
		if ( len == 0 ) {

			rc = -EFAULT;  /* アクセスできなかった */
			goto error_out;
		}
		env_len += len + 1;  /* 文字列長+NULLターミネート */
	}
	nr_envs = i + 1;  /* 環境変数の数 + NULL文字列 */

	/* argc, argv配列, environment配列, argv, environmentに対してスタックアラインメントで
	 * アクセスできるようにサイズを調整する
	 */
	array_len = roundup_align(
		sizeof(reg_type) + sizeof(char *) * nr_args + sizeof(char *) * nr_envs, 
		HAL_STACK_ALIGN_SIZE);
	argv_len = roundup_align(argv_len, HAL_STACK_ALIGN_SIZE);
	env_len = roundup_align(env_len, HAL_STACK_ALIGN_SIZE);

	 if ( argv_nrp != NULL )
		 *argv_nrp = nr_args;   /* 引数の個数を返却する     */

	 if ( env_nrp != NULL )
		 *env_nrp = nr_envs;    /* 環境変数の個数を返却する */

	if ( sizp != NULL )
		*sizp = array_len + argv_len + env_len;  /* 引数領域長を返却する */

	return  0;

error_out:
	return rc;
}

/**
   pidをキーにプロセス管理情報への参照を得る
   @param[in] target 検索対象プロセスのpid
   @return NULL 指定されたpidのプロセスが見つからなかった
   @return 見つかったプロセスのプロセス管理情報
   @note   プロセスへの参照をインクリメントするので返却後に
   proc_ref_dec()を呼び出すこと
 */
proc *
proc_find_by_pid(pid target){
	proc          *p;
	proc         key;
	intrflags iflags;
	
	key.id = target; /* キーとなるpidを設定 */

	/* プロセスDBのロックを獲得 */
	spinlock_lock_disable_intr(&g_procdb.lock, &iflags);

	p = RB_FIND(_procdb_tree, &g_procdb.head, &key); /* プロセス管理情報を検索 */

	if ( p != NULL )
		proc_ref_inc(p);  /* プロセス管理構造への参照をインクリメント */

	/* プロセスDBのロックを解放 */
	spinlock_unlock_restore_intr(&g_procdb.lock, &iflags);

	return p;  /* プロセス管理情報を返却 */
}
/**
   プロセスのマスタースレッドを取得する
   @param[in] target 検索対象プロセスのpid
   @return NULL 指定されたpidのプロセスが見つからなかった
   @return 見つかったプロセスのマスタースレッド
   @note   スレッドへの参照をインクリメントするので返却後に
   thr_ref_dec()を呼び出すこと
 */
thread *
proc_find_thread(pid target){
	bool         res;
	proc          *p;
	thread      *thr;
	proc         key;
	intrflags iflags;
	
	key.id = target; /* キーとなるpidを設定 */

	/* プロセスDBのロックを獲得 */
	spinlock_lock_disable_intr(&g_procdb.lock, &iflags);

	p = RB_FIND(_procdb_tree, &g_procdb.head, &key); /* プロセス管理情報を検索 */
	if ( p == NULL )
		goto unlock_out;

	res = proc_ref_inc(p);  /* スレッド獲得処理用の参照を獲得 */
	kassert( res );  /* 対象のスレッドが含まれるはずなので参照を獲得できる */

	/* プロセスDBのロックを解放 */
	spinlock_unlock_restore_intr(&g_procdb.lock, &iflags);

	thr = p->master;        /* マスタースレッド取得     */

	res = thr_ref_inc(thr); /*  スレッドの参照を取得    */
	kassert( res );

	res = proc_ref_dec(p);  /* スレッド獲得処理用の参照を獲得 */
	kassert( !res );  /* 最終参照ではないはず */

	return thr;  /* スレッド管理情報を返却 */

unlock_out:
	/* プロセスDBのロックを解放 */
	spinlock_unlock_restore_intr(&g_procdb.lock, &iflags);

	return NULL;
}

/**
   プロセスにスレッドを追加する
   @param[in] p   プロセス管理情報
   @param[in] thr スレッド管理情報
   @retval  0        正常終了
   @retval -ENOENT   プロセス削除中
 */
int
proc_add_thread(proc *p, thread *thr){
	bool         res;
	bool    is_first;
	intrflags iflags;

	if ( !proc_ref_inc(p) )  /* スレッドからの参照をインクリメント */
		return -ENOENT;  /* 削除中のプロセスにスレッドを追加しようとした */

	/* プロセス管理情報のロックを獲得 */
	spinlock_lock_disable_intr(&p->lock, &iflags);
	is_first = queue_is_empty(&p->thrque);

	queue_add(&p->thrque, &thr->proc_link);  /* スレッドキューに追加      */	
	thr->p = p;  /*  プロセスへのポインタを更新 */

	if ( is_first ) { /* 最初のスレッドだった場合 */

		p->master = container_of(queue_ref_top( &thr->p->thrque ),
		    thread, proc_link);  /* マスタースレッド更新 */

		/* 最初のスレッドからの参照分はプロセス生成時に加算済みなので
		 * 2重カウントしないように減算する
		 */
		res = proc_ref_dec(p);
		kassert( !res );  /* 上記で参照を得ているので最終参照ではない */
	}

	/* プロセス管理情報のロックを解放 */
	spinlock_unlock_restore_intr(&p->lock, &iflags);

	return  0;
}

/**
   プロセスからスレッドを削除する
   @param[in] p   プロセス管理情報
   @param[in] thr スレッド管理情報
   @retval  真    プロセスの最終スレッドを削除した
   @retval  偽    プロセスにスレッドが残存している
 */
bool
proc_del_thread(proc *p, thread *thr){
	bool          rc;
	bool         res;
	intrflags iflags;

	res = proc_ref_inc(p);  /* スレッド削除処理用の参照を獲得 */
	kassert( res );  /* 削除対象のスレッドが含まれるはずなので参照を獲得できる */

	/* プロセス管理情報のロックを獲得 */
	spinlock_lock_disable_intr(&p->lock, &iflags);

	queue_del(&p->thrque, &thr->proc_link);  /* スレッドキューから削除  */	

	/* プロセス管理情報のロックを解放 */
	spinlock_unlock_restore_intr(&p->lock, &iflags);

	rc = proc_ref_dec(p);  /* スレッド削除に伴う参照のデクリメント */
	kassert( !rc );  /* 上記で参照を得ているので最終参照ではないはず */

	/* プロセスキューが空でなく, 終了したスレッドがマスタースレッドの場合
	   マスタースレッドを更新する
	 */
	if ( !queue_is_empty(&p->thrque) && ( thr == thr->p->master ) )
		p->master = container_of(queue_ref_top( &thr->p->thrque ),
		    thread, proc_link);

	/** スレッドのアドレス空間離脱に伴うTLBのフラッシュ
	 */
	if ( thr->tinfo->cpu == ti_current_cpu_get() )
		hal_pgtbl_deactivate(p->pgt);  /* TLBのフラッシュ */
	else {
		/* TODO: TLBフラッシュを他コアに発行 */
	}
	rc = proc_ref_dec(p);  /* スレッド削除処理用の参照を解放 */

	return  rc;
}

/**
   スタックを割り当てる
   @param[in]  dest        割り当て先プロセス
   @param[in]  newsp       割り当て後のスタックポインタ
   @retval     0           正常終了
   @retval    -EFAULT      メモリアクセス不可
 */
int
proc_grow_stack(proc *dest, vm_vaddr newsp){
	int              rc;
	vm_vaddr   pg_start;
	vm_vaddr     pg_end;
	vm_vaddr     pg_cur;
	vm_prot    map_prot;
	vm_vaddr  map_paddr;
	vm_flags  map_flags;
	vm_size  map_pgsize;

	pg_start = truncate_align(newsp, PAGE_SIZE);    /* 割り当て対象開始アドレス */
	/* 最終割り当て対象ページ開始アドレス */
	pg_end = truncate_align(dest->segments[PROC_STACK_SEG].start - 1, PAGE_SIZE);
	
	for( pg_cur = pg_end; pg_cur >= pg_start; ) {

		rc = hal_pgtbl_extract(dest->pgt, pg_cur, &map_paddr, &map_prot, &map_flags,
		    &map_pgsize);
		if ( rc == 0 ) { /* 既にマップ済み */

			pg_cur = truncate_align(pg_cur, map_pgsize); /* ページの先頭を指す */
			pg_cur = truncate_align(pg_cur, PAGE_SIZE);  /* 次ページの先頭 */
			continue;
		}

		/* スタックページを割り当てる */
		rc = vm_map_userpage(dest->pgt, pg_cur, 
		    dest->segments[PROC_STACK_SEG].prot, VM_FLAGS_USER, PAGE_SIZE, PAGE_SIZE);
		if ( rc != 0 )
			goto error_out;

		/* 割当て済み先頭ページを更新 */
		dest->segments[PROC_STACK_SEG].start = pg_cur; 
		pg_cur -= PAGE_SIZE;  /* 次のページの割り当て */
	}

	return 0;

error_out:
	return rc;
}

/**
   引数をコピーする
   @param[in]  src         引数を読込むプロセス
   @param[in]  prot        スタックの保護属性
   @param[in]  argv        引数配列のアドレス
   @param[in]  environment 環境変数配列のアドレス
   @param[in]  dest        引数を書き込むプロセス
   @param[out] cursp       スタックポインタを指し示すポインタ変数のアドレス
   @param[out] argcp       引数の個数格納領域を返却域
   @param[out] argvp       引数の個数格納領域を返却域
   @param[out] envp        引数の個数格納領域を返却域
   @retval     0           正常終了
   @retval    -EFAULT      メモリアクセス不可
 */
int
proc_argument_copy(proc *src, vm_prot prot, const char *argv[], const char *environment[], 
    proc *dest, vm_vaddr *cursp, vm_vaddr *argcp, vm_vaddr *argvp, vm_vaddr *envp){
	int              rc;
	int               i;
	vm_vaddr         sp;
	char     **argv_ptr;
	char      **env_ptr;
	int         argv_nr;
	int          env_nr;
	vm_vaddr  cur_argcp;
	vm_vaddr  cur_argvp;
	vm_vaddr   cur_envp;
	char       *term[1];
	proc     *kern_proc;
	reg_type       argc;
	size_t          len;
	size_t          res;

	kern_proc = proc_kernel_process_refer(); /* カーネルプロセス */
	sp = *cursp;    /* 現在のスタック位置を取得 */
	term[0]	= NULL; /* NULLターミネート */

	/*
	 * 引数領域長を得る
	 */
	rc = calc_argument_areasize(src, argv, environment, &argv_nr, &env_nr, &len);
	if ( rc != 0 )
		goto error_out;  /* アクセス不能 */

	/* 引数領域のスタックポインタの先頭位置  */
	sp = truncate_align(sp - len, HAL_STACK_ALIGN_SIZE); 

	/* スタック伸張 */
	rc = proc_grow_stack(dest, sp);  
	if ( rc != 0 )
		goto error_out;  /* 伸張不能 */

	/* argc, argv, environmentをコピーする
	 */
	cur_argcp = sp;  /* argc保存領域 */
	/* argv[]配列の先頭アドレス */
	argv_ptr = (char **)(cur_argcp + sizeof(char *));  
	/* environment[]配列の先頭アドレス */
	env_ptr = (char **)((uintptr_t)argv_ptr + sizeof(char *) * argv_nr);

	/*
	 * argv[]のコピー
	 */

	/* 引数文字列の先頭アドレス */
	cur_argvp = roundup_align((uintptr_t)env_ptr + sizeof(char *) * env_nr, 
			      HAL_STACK_ALIGN_SIZE);
	for(i = 0; argv[i] != NULL; ++i) {

		len = vm_strlen(src->pgt, argv[i]);
		if ( len == 0 ) {

			rc = -EFAULT;  /* アクセスできなかった */
			goto error_out;
		}
		/* NULL終端を含めてコピーする */
		res = vm_memmove( dest->pgt, (void *)cur_argvp, src->pgt, 
		    (void *)argv[i], len + 1);
		if ( res != 0 ) {

			rc = -EFAULT;  /* アクセスできなかった */
			goto error_out;
		}

		/* 転送先のargv配列に記録する */
		res = vm_memmove( dest->pgt, (void *)&argv_ptr[i], dest->pgt, 
		    (void *)&cur_argvp, sizeof(char *));
		if ( res != 0 ) {

			rc = -EFAULT;  /* アクセスできなかった */
			goto error_out;
		}
		cur_argvp += len + 1;  /* 次の領域を指す */
	}

	/* 転送先のargv配列にNULLを記録 */
	res = vm_memmove( dest->pgt, (void *)&argv_ptr[i], dest->pgt, 
	    (void *)&term[0], sizeof(char *));
	if ( res != 0 ) {
		
		rc = -EFAULT;  /* アクセスできなかった */
		goto error_out;
	}

	argc = i;  /* argcを記憶 */

	/*
	 * environment[]のコピー
	 */
	cur_envp = roundup_align(cur_argvp, HAL_STACK_ALIGN_SIZE);
	for(i = 0; environment[i] != NULL; ++i) {

		len = vm_strlen(src->pgt, environment[i]);
		if ( len == 0 ) {

			rc = -EFAULT;  /* アクセスできなかった */
			goto error_out;
		}
		/* NULL終端を含めてコピーする */
		res = vm_memmove( dest->pgt, (void *)cur_envp, src->pgt, 
		    (void *)environment[i], len + 1);
		if ( res != 0 ) {

			rc = -EFAULT;  /* アクセスできなかった */
			goto error_out;
		}

		/* 転送先のenvironment配列に記録 */
		res = vm_memmove( dest->pgt, (void *)&env_ptr[i], kern_proc->pgt, 
		    (void *)&cur_envp, sizeof(char *));
		if ( res != 0 ) {

			rc = -EFAULT;  /* アクセスできなかった */
			goto error_out;
		}

		cur_envp += len + 1;  /* 次の領域を指す */
	}

	/* 転送先のenvironment配列にNULLを記録 */
	res = vm_memmove( dest->pgt, (void *)&env_ptr[i], dest->pgt, 
	    (void *)&term[0], sizeof(char *));
	if ( res != 0 ) {
		
		rc = -EFAULT;  /* アクセスできなかった */
		goto error_out;
	}

	/*
	 * argcを設定
	 */
	res = vm_memmove( dest->pgt, (void *)cur_argcp, kern_proc->pgt, 
	    (void *)&argc, sizeof(reg_type)); /* argcを設定       */
	if ( res != 0 ) {

		rc = -EFAULT;  /* アクセスできなかった */
		goto error_out;
	}
	
	*cursp = cur_argcp;     /* スタックを更新             */
	*argcp = cur_argcp;     /* 引数の個数を返却           */
	*argvp = (vm_vaddr)&argv_ptr[0];  /* 引数配列アドレスを返却     */
	*envp  = (vm_vaddr)&env_ptr[0];   /* 環境変数配列アドレスを返却 */

	return  0;

error_out:
	return rc;
}

/**
   カーネルプロセスを参照する
 */
proc *
proc_kernel_process_refer(void){

	return kern_proc;  /* カーネルプロセスへの参照を返却する */
}

/**
   ユーザプロセス用のプロセス管理情報を割り当てる
   @param[out] procp プロセス管理情報返却アドレス
   @retval     0     正常終了
   @retval    -ENOMEM メモリ不足
   @retval    -ENODEV ミューテックスが破棄された
   @retval    -EINTR  非同期イベントを受信した
   @retval    -EINVAL 不正な優先度を指定した
   @retval    -ENOSPC スレッドIDに空きがない
 */
int
proc_user_allocate(proc **procp){
	int             rc;
	proc     *new_proc;
	proc          *res;
	proc_segment  *seg;
	intrflags   iflags;

	/* プロセス管理情報を割り当てる
	 */
	rc = allocate_process_common(&new_proc);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto error_out;  /* メモリ不足 */
	}

	/* ページテーブルを割り当てる
	 */
	rc = pgtbl_alloc_user_pgtbl(&new_proc->pgt);
	if ( rc != 0 ) 
		goto free_proc_out;  

	new_proc->id = new_proc->pgt->asid;  /* アドレス空間IDをプロセスIDに設定 */

	/* ユーザスタックアドレスを設定する
	 */
	seg = &new_proc->segments[PROC_STACK_SEG];              /* スタックセグメント参照 */
	seg->start = PAGE_ROUNDUP(HAL_USER_END_ADDR);           /* スタック開始ページ */
	seg->end = PAGE_ROUNDUP(HAL_USER_END_ADDR);             /* スタック終了ページ */
	seg->prot = VM_PROT_READ|VM_PROT_WRITE;                 /* スタック保護属性   */
	seg->flags = VM_FLAGS_USER;                             /* マップ属性         */

        /* プロセス管理情報をプロセスツリーに登録 */
	spinlock_lock_disable_intr(&g_procdb.lock, &iflags);
	res = RB_INSERT(_procdb_tree, &g_procdb.head, new_proc);
	spinlock_unlock_restore_intr(&g_procdb.lock, &iflags);
	kassert( res == NULL );

	*procp = new_proc;  /* プロセス管理情報を返却 */
	
	return 0;

free_proc_out:
	slab_kmem_cache_free(new_proc); /* プロセス情報を解放する */

error_out:
	return rc;
}

/**
   プロセスへの参照を得る
   @param[in] p  プロセス管理情報
   @retval    真 プロセスへの参照を獲得できた
   @retval    偽 プロセスへの参照を獲得できなかった
 */
bool
proc_ref_inc(proc *p){

	/* プロセス終了中(プロセス管理ツリーから外れているスレッドの最終参照解放中)
	 * でなければ, 利用カウンタを加算し, 加算前の値を返す  
	 */
	return ( refcnt_inc_if_valid(&p->refs) != 0 );  /* 以前の値が0の場合加算できない */
}

/**
   プロセスへの参照を解放する
   @param[in] p  プロセス管理情報
   @retval 真 プロセスの最終参照者だった
   @retval 偽 プロセスの最終参照者でなかった
   @note LO プロセスDBロック, プロセスのロックの順にロックする
 */
bool
proc_ref_dec(proc *p){
	bool           res;
	proc     *proc_res;
	intrflags   iflags;

	/*  スレッドの最終参照者であればスレッドを解放する
	 */
	res = refcnt_dec_and_lock_disable_intr(&p->refs, &g_procdb.lock, &iflags);
	if ( res ) {  /* 最終参照者だった場合  */

		/* スレッドキューが空であることを確認する
		 */
		spinlock_lock(&p->lock);	
		kassert( queue_is_empty(&p->thrque) ); 
		spinlock_unlock(&p->lock);

		/* プロセスをツリーから削除 */
		proc_res = RB_REMOVE(_procdb_tree, &g_procdb.head, p);
		kassert( proc_res != NULL );

		/* プロセス管理ツリーのロックを解放 */
		spinlock_unlock_restore_intr(&g_procdb.lock, &iflags);

		free_user_process(p);  /* プロセスの資源を解放する  */
	}

	return res;
}

/**
   プロセス管理機構を初期化する
 */
void
proc_init(void){
	int rc;

	/* プロセス管理情報のキャッシュを初期化する */
	rc = slab_kmem_cache_create(&proc_cache, "proc cache", sizeof(proc),
	    SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	init_kernel_process();  /* カーネルプロセスを生成する */
}
