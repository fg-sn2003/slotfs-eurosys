echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
sudo rm -rf /mnt/pmem0/*
sudo umount /mnt/pmem0
sudo rmmod pmfs
sudo modprobe pmfs
sudo mount -t pmfs -o init /dev/pmem0 /mnt/pmem0
