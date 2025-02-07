sudo ndctl disable-namespace all
sudo ndctl destroy-namespace all --force
sudo ndctl create-namespace -m devdax 

echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
apt-get install libboost-all-dev ndctl numactl