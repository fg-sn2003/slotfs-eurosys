export SLOTFS_MKFS=1
rm -rf /dev/shm/slotfs_shm
# LD_PRELOAD=build/libslot.so fio -filename=\\fio-test -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs=4k -size=2G -name=test -thread -numjobs=1
LD_PRELOAD=build/libslot.so fio -directory=\\/ -fallocate=none -direct=0 -iodepth 1 -rw=write --overwrite=1 -ioengine=sync -bs=4k -size=3G -name=test -thread -numjobs=1
