top=.
include Makefile.inc
targets=kernel.elf kernel-dbg.elf kernel.asm kernel.map

subdirs=kern klib test tools
cleandirs=include ${subdirs}
kernlibs=klib/libklib.a kern/libklib.a test/libktest.a hal/hal/libhal.a
mconf=tools/kconfig/mconf

all:${targets}

start_obj =
ifeq ($(CONFIG_HAL),y)
start_obj += hal/hal/boot.o
subdirs += hal
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

kernel-dbg.elf: include/kern/autoconf.h subsystem ${start_obj}
ifeq ($(CONFIG_HAL),y)
	${CC} -static ${PIC_OPT_FLAGS} ${LDFLAGS}  $(shell echo ${CONFIG_HAL_LDFLAGS}) 	\
		-nostdlib -Wl,-T hal/hal/kernel.lds			\
		-o $@ ${start_obj} 				\
		-Wl,--start-group ${kernlibs} ${hallibs} -Wl,--end-group
else
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${start_obj} -Wl,--start-group ${kernlibs} -Wl,--end-group
endif

include/kern/autoconf.h: .config
	${RM} -f $@
	tools/kconfig/conf-header.sh .config > $@

.config: hal configs/Config.in ${mconf}
	@echo "Type make menuconfig beforehand"

menuconfig: hal configs/Config.in ${mconf}
	${RM} include/kern/autoconf.h
	${mconf} configs/Config.in || :

${mconf}:
	${MAKE} -C tools

subsystem: hal
	for dir in ${subdirs} ; do \
	${MAKE} -C $${dir} ;\
	done

run: hal kernel.elf
	${MAKE} -C hal/hal $@ ;\

run-debug: hal kernel.elf
	${MAKE} -C hal/hal $@ ;\

hal:
	${MAKE} -C include hal
	${MAKE} -C hal hal
	${MAKE} -C configs hal

clean:
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} clean ;\
	done
	${RM} *.o ${targets} *.tmp *.elf *.asm *.map *.iso

distclean:clean
	for dir in ${cleandirs} ; do \
	${MAKE} -C $${dir} distclean ;\
	done
	${RM} \#* *~ .config* _config GPATH GRTAGS GSYMS GTAGS *.log

dist: 
	${RM} ${ARCHIVE_NAME}.tar.gz
	${GIT} archive HEAD --format=tar.gz > ${ARCHIVE_NAME}.tar.gz

gtags:
	${GTAGS} -v

