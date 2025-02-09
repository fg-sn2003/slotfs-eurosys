#!/usr/bin/sh

WORK_DIR=$(cd "$( dirname "$0" )" && pwd)

cd "$WORK_DIR" || exit

../tools/AutoMacro/automacro.py --defs WRITE_PATTERN,"$1" --defs BLKS_CNT,"$2"

cd - || exit