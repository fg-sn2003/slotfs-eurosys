#!/usr/bin/sh

WORK_DIR=$(cd "$( dirname "$0" )" && pwd)

cd "$WORK_DIR" || exit

../tools/AutoMacro/automacro.py --defs ENABLE_GC_TEST_MODE,"" -f nova.h 
../tools/AutoMacro/automacro.py --defs EMU_PMEM_SIZE_GB,"$1" -f nova.h 

cd - || exit