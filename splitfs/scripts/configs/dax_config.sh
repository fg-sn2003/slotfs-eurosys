echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
sudo rm -rf /mnt/pmem0/*
sudo umount /mnt/pmem0
sudo rmmod ext4relink

cd ../../ext4relink || exit
make -j32
sudo insmod ext4relink.ko
cd - || exit

sudo mkfs.ext4 -F -b 4096 /dev/pmem0
sudo mount -t ext4relink -o dax /dev/pmem0 /mnt/pmem0
