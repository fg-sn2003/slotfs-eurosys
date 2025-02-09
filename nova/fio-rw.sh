sudo fio -filename="/mnt/pmem0/gc1" -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs=4k -size=64GB -name=test -numjobs=1 > output1.log&
sudo fio -filename="/mnt/pmem0/gc" -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs=4k -size=32GB -name=test -numjobs=1 > output2.log&
sudo fio -filename="/mnt/pmem0/gc2" -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs=4k -size=64GB -name=test -numjobs=1 > output3.log&
wait
