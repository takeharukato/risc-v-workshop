top=..
include ${top}/Makefile.inc

subdirs=klib kern dev hal

all: hal

hal:
	${RM} hal
	${LN} -s hal-${CPU} hal

clean: hal
	for dir in ${subdirs} ${HAL_INC_DIRS} ; do \
	${MAKE} -C $${dir} clean ;\
	done
	${RM} ${CLEAN_FILES}

distclean:hal clean
	for dir in ${subdirs} ${HAL_INC_DIRS} ; do \
	${MAKE} -C $${dir} distclean ;\
	done
	${RM} hal
	${RM} ${DIST_CLEAN_FILES} *~ autoconf.h
