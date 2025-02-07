import subprocess
import logging
import os
import pandas as pd
import re
import sys

def init_log():
    logging.basicConfig(
    filename='test.log',
    filemode='w',
    level=logging.INFO, 
    format='%(asctime)s - %(levelname)s - %(message)s')

def exec(command, nerf=False):
    logging.info("Running command: " + command)
    result = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    if result.returncode != 0 and not nerf:
        logging.error("Error running command: " + command)
        logging.error("Error: " + result.stderr)
    return result

def mount_splitfs():
    exec("sudo umount /mnt/pmem0", nerf=True)
    exec("make clean")
    exec("export LEDGER_DATAJ=1")
    exec("export LEDGER_POSIX=0")
    exec("export LEDGER_FIO=1")
    exec("sudo umount /mnt/pmem0")
    exec("sudo mkfs.ext4 -F -b 4096 /dev/pmem0")
    exec("sudo mount -t ext4 -o dax /dev/pmem0 /mnt/pmem0")

io_workloads = ["sw", "rw", "sow", "row", "sr", "rr"]
jobs=range(1, 17)
df = pd.DataFrame(columns=["fs", "jobs", "io_worklaods", "bw"])
mount_path = "/mnt/pmem0"
splitfs_path = "/home/zyf/nvmfs/wofs/splitfs/splitfs"

def run_multithread():
    global df
    common_fio_command = "fio" + " --fallocate=none --direct=0 --iodepth=1 --ioengine=sync --name=test --bs=4k"

    for j in jobs:
        for io_workload in io_workloads:
            fio_command = common_fio_command
            if io_workload == "sw":
                fio_command += " --rw=write --size=2G " 
            elif io_workload == "rw":
                fio_command += " --rw=randwrite --size=2G "
            elif io_workload == "sow":
                fio_command += " --rw=write --size=2G --filesize=512M --overwrite=1"
            elif io_workload == "row":
                fio_command += " --rw=randwrite --size=2G --filesize=512M  --overwrite=1"
            elif io_workload == "sr":
                fio_command += " --rw=read --size=2G "
            elif io_workload == "rr":
                fio_command += " --rw=randread --size=2G "
            else:
                raise ValueError("Invalid pattern")
            
            if j is 1:
                fio_command += " --filename=" + mount_path + "/test"
            else:
                fio_command += " --directory=" + mount_path
            
            logging.info("Running FIO with " + io_workload)

            fio_command += " --numjobs=" + str(j)

            mount_splitfs()


            env_command = "export LD_LIBRARY_PATH=" + splitfs_path \
                                + "&& export NVP_TREE_FILE=" + splitfs_path + "/bin/nvp_nvp.tree"

            fio_command = "LD_PRELOAD=" + splitfs_path + "/libnvp.so " + fio_command

            fio_command = env_command + "&&" + fio_command

            fio_result = exec(fio_command) 

            print(fio_result.stdout)

            if io_workload == "sr" or io_workload == "rr":
                match = re.search(r'READ: bw=(\d+)MiB/s', fio_result.stdout)      
            else:
                match = re.search(r'WRITE: bw=(\d+)MiB/s', fio_result.stdout)
            bw = match.group(1)
            
            df = df.append({"fs": "splitfs", "jobs": j, "io_workload": io_workload, "bw": bw}, ignore_index=True)
            df.to_csv("multithread.csv", index=False)


def run_singlethread():
    return

if __name__ == "__main__":
    init_log()
    run_multithread()