top=.
include Makefile.inc
targets=kernel.elf kernel-dbg.elf kernel.asm kernel.map
fsimg_obj=$(patsubst %.img,%.o, ${FSIMG_FILE})
fsimg_objfile=${top}/fs/${fsimg_obj}
kern_subdirs=kern klib fs hal test
supdirs=tools doxygen
subdirs=kern klib fs hal test
cleandirs=include ${subdirs} ${supdirs}
distcleandirs=${cleandirs} configs
kernlibs=klib/libklib.a kern/libkern.a fs/libfs.a test/libktest.a hal/hal/libhal.a 
mconf=tools/kconfig/mconf

ifeq ($(CONFIG_FORCE_UPDATE_GTAGS),y)
all: gtags ${targets}
else
all: ${targets}
endif

start_obj =
ifeq ($(CONFIG_HAL),y)
start_obj += hal/hal/boot.o
endif
start_obj += kern/main.o

kernel.asm: kernel-dbg.elf
	${RM} $@
	${OBJDUMP} -S $< > $@

kernel.map: kernel.elf
	${RM} $@
	${NM} $< |${SORT} > $@

kernel.elf: kernel-dbg.elf
	${CP}	$< $@
	${STRIP} -g $@

kernel-dbg.elf: include/kern/autoconf.h include/klib/asm-offset.h subsystem ${start_obj} \
	${fsimg_objfile}
ifeq ($(CONFIG_HAL),y)
	${CC} -static ${PIC_OPT_FLAGS} ${CFLAGS} ${LDFLAGS}  $(shell echo ${CONFIG_HAL_LDFLAGS}) 	\
		-nostdlib -Wl,-T hal/hal/kernel.lds			\
		-o $@ ${start_obj} ${fsimg_objfile}	                \
		-Wl,--start-group ${kernlibs} -Wl,--end-group
else
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${start_obj} ${fsimg_objfile} \
	-Wl,--start-group ${kernlibs} -Wl,--end-group
endif

#
# asm-offsetヘッダ生成ルール
#
include/klib/asm-offset.h: hal/hal/asm-offset.s
	tools/asmoffset/gen-asm-offset.py \
	-i $< -o $@

hal/hal/asm-offset.s: hal/hal/asm-offset.c tools/asmoffset/gen-asm-offset.py
	${CC} ${ASM_OFFSET_CFLAGS} -S -o $@ $<

hal/hal/asm-offset.c: hal

include/kern/autoconf.h: .config
	${RM} $@
	tools/kconfig/conf-header.sh .config > $@

.config: configs/Config.in
	@echo "Type make menuconfig beforehand" 
	@exit 1

menuconfig: configs/Config.in ${mconf}
	${RM} include/kern/autoconf.h
	${mconf} $< || :

${mconf}: 
	${MAKE} -C tools

configs/Config.in: configs/hal/Config.in

configs/hal/Config.in: hal

subsystem: hal
	for dir in ${subdirs} ; do \
	${MAKE} -C $${dir} ;\
	done

mkfsimg:
${fsimg_objfile}:
	${MAKE} -C fs ${fsimg_obj} ;

run: hal kernel.elf
	${MAKE} -C hal/hal $@ ;\

run-debug: hal kernel.elf
	${MAKE} -C hal/hal $@ ;\

doxygen: 
	${MAKE} -C doxygen

hal:
	${MAKE} -C include hal
	${MAKE} -C hal hal
	${MAKE} -C configs hal

clean:
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} clean ;\
	done
	${RM} ${CLEAN_FILES} ${targets} *.tmp *.elf *.asm *.map *.iso include/klib/asm-offset.h

distclean:clean
	for dir in ${distcleandirs} ; do \
	${MAKE} -C $${dir} distclean ;\
	done
	${RM} ${DIST_CLEAN_FILES} include/kern/autoconf.h

dist: 
	${RM} ${ARCHIVE_NAME}.tar.gz
	${GIT} archive HEAD --format=tar.gz > ${ARCHIVE_NAME}.tar.gz

gtags:
	${RM} ${GTAGS_CLEAN_FILES}
	${GTAGS} -v

ifeq ($(CONFIG_PROFILE),y)
gcov: run
	for dir in ${subdirs} ; do \
		${MAKE} -C $${dir} $@ ;\
	done
lcov: gcov
	${RM} ${LCOV_CLEAN_FILES}
	${LCOV} -c -d . -o tmp-kernel-cov.linfo
	${LCOV} -r tmp-kernel-cov.linfo *docs*  \
	     -r tmp-kernel-cov.linfo *tools* \
	     -r tmp-kernel-cov.linfo *test*  \
	     -o kernel-cov.linfo
	${RM} -fr ${LCOV_DIR}
	${MKDIR} -p ${LCOV_DIR}
	${GENHTML} kernel-cov.linfo --output-directory ${LCOV_DIR} --show-details
	${TAR} zcvf ${LCOV_DIR}.tar.gz ${LCOV_DIR}
	${RM} ${LCOV_CLEAN_FILES}
	${RM} -fr ${LCOV_DIR}
else
gcov: 
lcov:
endif

loc:
	tempfile=`mktemp`;					\
	echo ${supdirs}|sed  's|\s\+|\n|g' > $${tempfile};	\
	${CLOC} --exclude-lang="D" --exclude-list-file=$${tempfile} --by-file --vcs=git; \
	${RM} $${tempfile};


