#!/bin/sh

decode_trace(){
    local dbgfile
    local line
    local info
    local addr
    
    line=$1
    dbgfile=$2
    addr=`echo ${line}|awk -F ' ' '{print $2;}'`
    info=`${CROSS_COMPILE}addr2line -e ${dbgfile} ${addr}`
    echo "${line} ${info}";

}

show_log(){
    local dbgfile
    local file
    local line
    local in_trace
    
    file=$1
    dbgfile=$2

    in_trace="no"
    
    cat ${file}|while read line
    do
	if [ "x${in_trace}" != "xno" ]; then
	    decode_trace "${line}" "${dbgfile}"
	else
	    echo ${line}
	fi
	echo ${line} | grep "Backtrace:" 1> /dev/null 2> /dev/null
	if [ $? -eq 0 ]; then
	    in_trace="yes"
	fi
    done
}

main(){
    local logfile
    local elffile
    
    if [ $# -ne 2 ]; then
	echo "Usage: decode-trace.sh exception-log.txt kernel-dbg.elf"
	exit 1
    fi

    logfile=$1
    elffile=$2
    show_log "${logfile}" "${elffile}"
}

main $@

