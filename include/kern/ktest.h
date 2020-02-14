/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  kernel test routine                                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_KTEST_H)
#define _KERN_KTEST_H

#define KTEST_MAX_TEST_NR (512)  /** テストプログラム当たりの最大テスト数  */

struct _ktest_stats;

typedef void (*ktest_func_type)(struct _ktest_stats *_sp, void *_arg); /*< テスト実施関数 */
typedef void (*tst_start_func_type)(void *_arg);  /*< テスト開始関数 */

/**
 * テスト定義
 */
typedef struct _ktest_def{
	ktest_func_type  func;  /*< テスト関数  */
	void             *arg;  /*< テスト引数  */	
}ktest_def;

/**
 *   テスト配列
 */
typedef struct _ktest_stats{
	char          *name;   /*< テスト名          */
	int32_t     defined;   /*<  試験数           */
	int32_t      tested;   /*<  実施試験数       */
	int32_t unsupported;   /*<  試験対象外数     */
	int32_t  unresolved;   /*<  結果不明試験数   */
	int32_t      failed;   /*<  失敗試験数       */
	int32_t     xfailed;   /*<  意図的失敗試験数 */
	int32_t      passed;   /*<  成功試験数       */
	ktest_def tests[KTEST_MAX_TEST_NR];  /*< テスト  */
}ktest_stats;

#define KTEST_INITIALIZER						\
	{								\
	.defined = 0,							\
	.tested = 0,							\
	.unsupported = 0,						\
	.unresolved = 0,						\
	.failed = 0,						        \
	.xfailed = 0,						        \
	.passed = 0,						        \
	 }
/**
   テストを定義する
   @param[in] _sp   テスト配列のアドレス
   @param[in] _name テスト名
   @param[in] _func テスト実施関数
   @param[in] _arg  テスト実施関数への引数
 */
#define ktest_def_test(_sp, _name, _func, _arg) do{			\
		((ktest_stats *)(_sp))->name = (_name);			\
		((ktest_def *)&((_sp)->tests[(_sp)->defined]))->func =	\
			(ktest_func_type)(_func);			\
		((ktest_def *)&((_sp)->tests[(_sp)->defined]))->arg =	\
			(_arg);						\
		(_sp)->defined++;					\
	}while(0)

/**
   サブテストを追加し成功したことを記録する
   @param[in] _sp     テスト配列のアドレス
   @param[in] _name   テスト名
 */
#define ktest_add_pass(_sp, _name) do{					\
	struct _ktest_stats *st = (_sp);                                \
									\
	kprintf("Runnning sub-test [%s] pass\n", _name);		\
	++st->passed;							\
	++st->tested;							\
	}while(0)

/**
   サブテストを追加し失敗したことを記録する
   @param[in] _sp     テスト配列のアドレス
   @param[in] _name   テスト名
 */
#define ktest_add_fail(_sp, _name) do{					\
	struct _ktest_stats *st = (_sp);                                \
									\
	kprintf("Running sub-test [%s] fail\n", _name);			\
	++st->failed;							\
	++st->tested;							\
	kprintf("fail: [file:%s func %s line:%d ]\n",			\
		__FILE__, __func__, __LINE__);				\
	}while(0)

/**
   サブテストを追加し意図的に失敗したことを記録する
   @param[in] _sp     テスト配列のアドレス
   @param[in] _name   テスト名
 */
#define ktest_add_xfail(_sp, _name) do{					\
	struct _ktest_stats *st = (_sp);                                \
									\
	kprintf("Running sub-test [%s] xfail\n", _name);		\
	++st->xfailed;							\
	++st->tested;							\
	kprintf("xfail: [file:%s func %s line:%d ]\n",			\
		__FILE__, __func__, __LINE__);				\
	}while(0)

/**
   テストが成功したことを記録する
   @param[in] _sp   テスト配列のアドレス
 */
#define ktest_pass(_sp) do{						\
	struct _ktest_stats *__st = (_sp);                              \
									\
	++__st->passed;							\
	++__st->tested;							\
	}while(0)

/**
   テストが失敗したことを記録する
   @param[in] _sp   テスト配列のアドレス
 */
#define ktest_fail(_sp) do{						\
	struct _ktest_stats *__st = (_sp);                              \
									\
	++__st->failed;							\
	++__st->tested;							\
	kprintf("fail: [file:%s func %s line:%d ]\n",			\
		__FILE__, __func__, __LINE__);				\
	}while(0)

/**
   テストが意図的に失敗したことを記録する
   @param[in] _sp   テスト配列のアドレス
 */
#define ktest_xfail(_sp) do{						\
	struct _ktest_stats *__st = (_sp);                              \
									\
	++__st->xfailed;						\
	++__st->tested;							\
	kprintf("xfail: [file:%s func %s line:%d ]\n",			\
		__FILE__, __func__, __LINE__);				\
	}while(0)

/**
   テストを実行する
   @param[in] _sp   テスト配列のアドレス
 */
#define ktest_run(_sp) do{						\
	int __i;							\
	struct _ktest_stats *__st = (_sp);				\
	ktest_def *__tst;						\
									\
	for(__i = 0; __st->defined > __i; ++__i) {			\
		__tst = ((ktest_def *)&__st->tests[__i]);		\
	kprintf("Running test [%s] \n",	__st->name);			\
	if ( __tst->func != NULL )					\
		__tst->func(__st, __tst->arg);				\
	}								\
	kprintf("Test: [pass/fail/xfail/total] = [%d/%d/%d/%d]\n",	\
	    __st->passed, __st->failed, __st->xfailed, __st->tested);	\
	kprintf("TestRes,%s,%d,%d,%d,%d\n",				\
	    __st->name, __st->passed, __st->failed, __st->xfailed, __st->tested); \
	}while(0)

int tflib_kernlayout_init(void);
void tflib_kernlayout_finalize(void);
int tflib_kvaddr_to_pfn(void *_kvaddr, obj_cnt_type *_pfnp);
int tflib_pfn_to_kvaddr(obj_cnt_type _pfn, void **_kvaddrp);
vm_paddr tflib_kern_straight_to_phy(void *_kvaddr);
void *tflib_phy_to_kern_straight(vm_paddr _paddr);
void tst_spinlock(void);
void tst_atomic(void);
void tst_atomic64(void);
void tst_fixedpoint(void);
void tst_memset(void);
void tst_vmcopy(void);
void tst_rv64pgtbl(void);
void tst_vmstrlen(void);
void tst_vmmap(void);
void tst_irqctrlr(void);
void tst_pcache(void);
void tst_cpuinfo(void);
void tst_proc(void);
void tst_thread(void);
void tst_mutex(void);
void tst_bswap(void);
void tst_minixfs(void);
#endif  /*  _KERN_KTEST_H  */
