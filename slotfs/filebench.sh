export SLOTFS_MKFS=1
rm -rf /dev/shm/slotfs_shm
LD_PRELOAD=build/libslot.so filebench -f ../tools/fbscripts/fileserver.f
# strace -o output -f filebench -f ../tools/fbscripts/fileserver.f
