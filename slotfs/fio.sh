export SLOTFS_MKFS=1
rm -rf /dev/shm/slotfs_shm
# LD_PRELOAD=build/libslot.so fio -filename=\\fio-test -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs=4k -size=1M -name=test -thread -numjobs=1
# LD_PRELOAD=build/libslot.so fio -directory=\\/ -fallocate=none -direct=0 -iodepth 1 -rw=write --overwrite=1 -ioengine=sync -bs=4k -size=1M -name=test -thread -numjobs=8
# LD_PRELOAD=build/libslot.so ../../tool/fio/fio -filename=\\fio-test -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs=4k -size=1M -name=test -thread -numjobs=1
LD_PRELOAD=build/libslot.so ../../tool/fio-3.32/fio -directory=\\/ -fallocate=none -direct=0 -iodepth 1 -rw=write --overwrite=1 -ioengine=sync -bs=4k -size=1M -name=test -thread -numjobs=8
# LD_PRELOAD=build/libslot.so fio
# env LD_PRELOAD=build/libslot.so strace -o output-slot -f ../../tool/bin/fio -filename=\\fio-test -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs=4k -size=1M -name=test -thread -numjobs=1
# strace -o output -f ../../tool/bin/fio -filename=\\fio-test -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs=4k -size=1M -name=test -thread -numjobs=1
# LD_PRELOAD=build/libslot.so ../../tool/bin/fio -directory=\\/ -fallocate=none -direct=0 -iodepth 1 -rw=write --overwrite=1 -ioengine=sync -bs=4k -size=1M -name=test -thread -numjobs=8
# LD_PRELOAD=build/libslot.so fio -directory=dir -fallocate=none -direct=0 -iodepth 1 -rw=read -ioengine=sync -bs=4k -size=1G -name=readtest -thread -numjobs=8
