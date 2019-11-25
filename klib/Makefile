top=..
include ${top}/Makefile.inc

objects=atomic.o atomic64.o backtrace.o bitops.o early-kheap.o early-malloc.o em-memalign.o epoch_time.o kprintf.o list.o memchr.o memcmp.o memcpy.o memmove.o memset.o mt19937.o queue.o refcount.o slist.o statcnt.o strcasecmp.o strcat.o strchr.o strcmp.o strcpy.o strcspn.o strlen.o strncasecmp.o strncat.o strncmp.o strncpy.o strnlen.o strrchr.o strspn.o strstr.o strtok_r.o

depends = $(patsubst %.o,%.d,${objects})

lib=libklib.a

all: ${depends} ${lib}

${lib}: ${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@

gcov:
ifeq ($(CONFIG_PROFILE),y)
	${GCOV} ${GCOV_OPTS} $(patsubst %.o,%.gcda,${objects})
endif

clean:
	${RM} ${CLEAN_FILES} ${lib}

distclean: clean
	${RM} ${DIST_CLEAN_FILES}

-include *.d

