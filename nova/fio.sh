sudo fio -filename="/mnt/pmem0/gc" -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs=4k -size=64GB -name=test -numjobs=1