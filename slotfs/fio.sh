export SLOTFS_MKFS=1
rm -rf /dev/shm/slotfs_shm
LD_PRELOAD=build/libslot.so fio -filename=\\fio-test -fallocate=none -direct=0 -iodepth 1 -rw=read -ioengine=sync -bs=4k -size=2G -name=test -thread -numjobs=1
