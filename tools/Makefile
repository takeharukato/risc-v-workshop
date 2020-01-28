top=..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
subdirs-build=asmoffset
subdirs=${subdirs-build} kconfig

all: kconfig/mconf
	for dir in ${subdirs-build} ; do 	\
		${MAKE} -C $${dir} all	;	\
	done

kconfig/mconf:
	cd kconfig && ${MAKE} mconf

gcov:

clean:
	for dir in ${subdirs} ; do 	\
		${MAKE} -C $${dir} clean;	\
	done
	cd kconfig && $(MAKE) clean
	${RM} ${CLEAN_FILES}

distclean:clean
	for dir in ${subdirs} ; do 	\
		${MAKE} -C $${dir} distclean;	\
	done
	cd kconfig && $(MAKE) distclean
	${RM} ${DIST_CLEAN_FILES}
