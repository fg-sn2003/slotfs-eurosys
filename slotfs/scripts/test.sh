export SLOTFS_MKFS=1
rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/link_test
