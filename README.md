# SlotFS
## Introduction
Source code for Eurosys'25 paper: ***Overcoming the Last Mile between Log-Structured File Systems and Persistent Memory via Scatter Logging***.

SlotFS is a log structure file system for persistent memory. SlotFS leverages scatter logging techniques to eliminate garbage collection and to improve file system performance.

<details>
<summary>Abstract</summary>
Log-structured file system (LFS) has been a popular option for persistent memory (PM) for its high write performance and lightweight crash consistency protocols. However, even with PM's byte-addressable I/O interface, existing PM LFSes still maintain contiguous space for log locality while using heavy garbage collection (GC) to synchronously reclaim space, causing up to 50% I/O performance degradation on PM. Thus, there exists a last-mile problem between the contiguous space management of LFS (that induces GC) and the non-contiguous byte-addressability of PM.

To overcome this, we propose a novel scatter logging technique called SLOT. The core idea is to efficiently manage non-contiguous log entries on byte-addressable PM to prevent GC in LFS. Specifically, SLOT scatters log entries across PM and manages them in a per-entry granularity, thereby enabling the immediate reallocation of invalidated entries and eliminating GC overheads. SLOT further introduces an array of techniques to exploit PM write buffer efficiency to fully unleash PM I/O performance potential. We implement SlotFS to realize the efficiency of SLOT. Experimental results driven by synthetic and real-world workloads show that SlotFS significantly outperforms state-of-the-art PM file systems. Compared to two representative PM LFSes, NOVA
and MadFS, SlotFS achieves 27%–47% and 59%–175% performance improvement under a series of real-world workloads.
</details>



## Requirement
Hardware: Intel Xeon CPU equiped with Intel Optan NVDIMM

Software: Kernel with SplitFS and Hodor modification.

## Tested environment
- CPU: Intel(R) Xeon(R) Gold 5218 CPU @ 2.3GHz
- DRAM: 128 GiB DDR4 DRAM
- PMEM: 2 x 256GB Intel Optane DCPMM
- Kernel: Linux 5.1.0
- Distribution: CentOS Stream 8

## Prerequisites
(***For AE evaluator:The test environment has already been configured.***)

Run these commands to configre intel optane persistent memory, and reboot
```shell
sudo ndctl disable-namespace all
sudo ndctl destroy-namespace all --force
#/dev/dax1.0 for SlotFS
sudo ndctl create-namespace -m devdax
#/dev/pmem0 for others
sudo ndctl create-namespace -m fsdax
ndctl list --region=0 --namespaces --human --idle
```



## Build
All file systems will be automatically compiled by test scripts.

If you want to compile SlotFS.
```shell
cd slotfs
make -j$(nproc)
cd ..
```

## Minimal Workload
After you compile SlotFS, you can run test with slotfs by ***LD_PRELOAD***.

### Minimal Workload
```shell
cd slotfs
export SLOTFS_MKFS=1    #Trigger build-in MKFS.
LD_PRELOAD=./build/libslot.so ./tests/hello
# Sample output:
# Initialize SlotFS
# Hello, World!
# Unload Slotfs
```

### Basic functional Tests
```shell
# In slotfs directory
./scripts/test_all.sh
# Sample output:
# Initialize SlotFS
# xxx test pass
# Unload Slotfs
```


## Run
(***For AE evaluator: We are still preparing the testing and plotting scripts. We will complete all the scripts before the kick-the-tire deadline. Sorry for the inconvenience.***)

### One-click command
We have provided a one-click script to execute all the tests and plots the result. It will take about 6 hours in our machine.
```shell
bash ./tests/run_all.sh
# The output are saved in ./tests/results
```
### Detailed Experiments
You can also perform individual tests if needed.
```shell
cd ./tests/<the experiment you want to run>
bash test.sh
# The output are saved in performance-table
# Use the plot.ipynb to plot the results
```



## Directory Structure
- [`nova/`](nova): Source code for NOVA
- [`splitfs/`](splitfs): Source code for SplitFS
- [`pmfs/`](pmfs): Source code for PMFS
- [`madfs/`](madfs): Source code for MadFS
- [`slotfs/`](madfs): Source code for SlotFS
    - [`slotfs/lib.c`](slotfs/lib.c): The entry point and API wrapper of slotfs
    - [`slotfs/slotfs.c`](slotfs/slotfs.c): Initialization and unloading of SlotFS instances
    - [`slotfs/mkfs.c`](slotfs/mkfs.c): SlotFS mkfs implementation
    - [`slotfs/runtime.c`](slotfs/runtime.c): Per-application runtime
    - [`slotfs/slot_vfs.c`](slotfs/slot_vfs.c): Path resolution functionality
    - [`slotfs/slotfs_func.c`](slotfs/slotfs_func.c): SlotFS API implementation
    - [`slotfs/shm.c`](slotfs/shm.c):  Shared memory area initialization
    - [`slotfs/slot.c`](slotfs/slot.c):  Slot bitmap allocator
    - [`slotfs/journal.c`](slotfs/journal.c): Journal for crash consistency
    - [`slotfs/recover.c`](slotfs/recover.c):  Crash recover process
    - [`slotfs/gather.c`](slotfs/gather.c): Gather thread
    - [`slotfs/inode.c`](slotfs/inode.c): Slotfs inode
    - [`slotfs/arena.c`](slotfs/arena.c): Arena allocator
    - [`slotfs/heap/`](slotfs/heap/): Shm memory heap allocator
    - [`slotfs/tests/`](slotfs/tests/): Simple tests
- [`tests/`](tests): Experiment tests
- [`tools/`](tools): Tools for Experiment

