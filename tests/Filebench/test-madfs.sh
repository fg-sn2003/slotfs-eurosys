source "../common.sh"
ABS_PATH=$(where_is_script "$0")
TOOLS_PATH=$ABS_PATH/../../tools
FSCRIPT_PRE_FIX=$ABS_PATH/../../tools/fbscripts
BOOST_DIR=$ABS_PATH/../../splitfs/splitfs
SLOTFS_DIR=$ABS_PATH/../../slotfs
MADFS_DIR=$ABS_PATH/../../madfs

TABLE_NAME="$ABS_PATH/performance-table-madfs"
table_create "$TABLE_NAME" "file_system file_bench threads iops"
mkdir -p "$ABS_PATH"/DATA

# FILE_SYSTEMS=("SLOTFS")
FILE_SYSTEMS=("MadFS" "SLOTFS")
FILE_BENCHES=( "fileserver.f" "varmail.f" "webserver.f" "webproxy.f" )
THREADS=( 1 )

for file_system in "${FILE_SYSTEMS[@]}"; do
    for fbench in "${FILE_BENCHES[@]}"; do
        mkdir -p "$ABS_PATH"/DATA/"$fbench"
        for thread in "${THREADS[@]}"; do
            cp -f "$FSCRIPT_PRE_FIX"/"$fbench" "$ABS_PATH"/DATA/"$fbench"/"$thread" 
            sed_cmd='s/set $nthreads=.*$/set $nthreads='$thread'/g' 
            sed -i "$sed_cmd" "$ABS_PATH"/DATA/"$fbench"/"$thread"
            
            if [[ "${file_system}" == "SLOTFS" ]]; then
                sed -i 's|set $dir=.*$|set $dir=mnt|' "$ABS_PATH"/DATA/"$fbench"/"$thread"
            else
                sed -i 's|set $dir=.*$|set $dir=/mnt/pmem0|' "$ABS_PATH"/DATA/"$fbench"/"$thread"
            fi

            if [[ "${file_system}" == "SLOTFS" ]]; then
                bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                export SLOTFS_MKFS=1
                rm /dev/shm/slotfs_shm
                LD_PRELOAD=$SLOTFS_DIR/build/libslot.so filebench -f "$ABS_PATH"/DATA/"$fbench"/"$thread" | tee "$ABS_PATH"/DATA/"$fbench"/"$file_system"-"$thread"
            elif [[ "${file_system}" == "MadFS" ]]; then
                bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                rm -rf /dev/shm/*
                export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
                LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so filebench -f "$ABS_PATH"/DATA/"$fbench"/"$thread" | tee "$ABS_PATH"/DATA/"$fbench"/"$file_system"-"$thread"
            fi

            iops=$(filebench_attr_iops "$ABS_PATH"/DATA/"$fbench"/"$file_system"-"$thread")

            table_add_row "$TABLE_NAME" "$file_system $fbench $thread $iops"


        done
    done
done

