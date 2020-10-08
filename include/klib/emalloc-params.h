#if !defined(_EMALLOC_PARAMS_H)
#define _EMALLOC_PARAMS_H

#include <hal/hal-page.h>  /*  HAL_PAGE_SIZE  */

#include <klib/early-kheap.h>  /*  ekheap_sbrk  */

#define MSPACES         (0)
#define ONLY_MSPACES    (0)
#define USE_LOCKS       (0)
#define USE_SPIN_LOCKS  (0)
#define USE_DL_PREFIX   (1)
#define ABORT          do{}while(1)
#define MALLOC_FAILURE_ACTION do{}while(0)
#define HAVE_MORECORE   (1)
#define MORECORE        ekheap_sbrk
#define HAVE_MMAP       (0)
#define HAVE_MREMAP     (0)
#define MMAP_CLEARS     (0)
#define USE_BUILTIN_FFS (0)
#define malloc_getpagesize (HAL_PAGE_SIZE)
#define USE_DEV_RANDOM  (0)
#define NO_MALLINFO     (0)
#define NO_MALLOC_STATS (1)  /* Avoid using printf family */
#define LACKS_TIME_H    (1)  /* Avoid using time(3) */
#define LACKS_STDLIB_H     (1)  /* Avoid using SSE */
#define USE_EMALLOC_PREFIX (1)

#if defined (USE_EMALLOC_PREFIX)
#define dlcalloc               ecalloc
#define dlfree                 efree
#define dlmalloc               emalloc
#define dlmemalign             ememalign
#define dlposix_memalign       _em_posix_memalign
#define dlrealloc              erealloc
#define dlrealloc_in_place     erealloc_in_place
#define dlvalloc               evalloc
#define dlpvalloc              epvalloc
#define dlmallinfo             emallinfo
#define dlmallopt              emallopt
#define dlmalloc_trim          emalloc_trim
#define dlmalloc_stats         emalloc_stats
#define dlmalloc_usable_size   emalloc_usable_size
#define dlmalloc_footprint     emalloc_footprint
#define dlmalloc_max_footprint emalloc_max_footprint
#define dlmalloc_footprint_limit emalloc_footprint_limit
#define dlmalloc_set_footprint_limit emalloc_set_footprint_limit
#define dlmalloc_inspect_all   emalloc_inspect_all
#define dlindependent_calloc   eindependent_calloc
#define dlindependent_comalloc eindependent_comalloc
#define dlbulk_free            ebulk_free
#endif /* USE_EMALLOC_PREFIX */

#endif /*  _EMALLOC_PARAMS_H */
