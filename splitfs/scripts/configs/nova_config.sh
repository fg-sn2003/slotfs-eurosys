echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
sudo rm -rf /mnt/pmem0/*
sudo umount /mnt/pmem0
sudo rmmod nova
sudo modprobe nova
#sudo modprobe nova measure_timing=1 inplace_data_updates=1
#sudo modprobe nova inplace_data_updates=1
sudo mount -t NOVA -o init /dev/pmem0 /mnt/pmem0
