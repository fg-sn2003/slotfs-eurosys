set environment LD_PRELOAD ./build-debug/libmadfs.so
set breakpoint pending on
b madfs_ctor
run