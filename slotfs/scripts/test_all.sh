export SLOTFS_MKFS=1
rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/link_test

export SLOTFS_MKFS=1
rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/symlink_test

export SLOTFS_MKFS=1
rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/mkdir_test

export SLOTFS_MKFS=1
rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/rw_test

export SLOTFS_MKFS=1
rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/rename_test

export SLOTFS_MKFS=1
rm /dev/shm/slotfs_shm
LD_PRELOAD=./build/libslot.so ./tests/release_test