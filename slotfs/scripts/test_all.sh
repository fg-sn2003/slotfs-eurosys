export SLOTFS_MKFS=1
rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/link_test

rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/symlink_test

rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/mkdir_test

rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/rw_test

rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/rename_test

rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/release_test