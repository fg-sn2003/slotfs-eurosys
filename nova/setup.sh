#!/usr/bin/bash



#SECTION: Color Preset
CLR_BLACK="\033[30m"
CLR_RED="\033[31m"
CLR_GREEN="\033[32m"
CLR_YELLOW="\033[33m"
CLR_BLUD="\033[34m"
CLR_PURPLE="\033[35m"
CLR_BLUE="\033[36m"
CLR_GREY="\033[37m"

CLR_END="\033[0m"
#!SECTION


ORIGINAL=$PWD
WORK_DIR=$(dirname "$0")
MNT_POINT=/mnt/pmem0

# build project
cd "$WORK_DIR" || exit
sudo make -j"$(nproc)"
sudo dmesg -C

# parse config
if [ ! "$1" ]; then
    config_path="./config.example.json"
else 
    config_path="$1"
fi

config_json=$(cat "$config_path")

function get_modules_options () {
    echo "$config_json" | jq -r ".modules.$1"
}

function get_fs_options () {
    echo "$config_json" | jq -r ".fs.$1"
}


fs_init=$(get_fs_options init)
fs_snapshot=$(get_fs_options snapshot) 	  
fs_data_cow=$(get_fs_options data_cow) 	    
fs_wprotect=$(get_fs_options wprotect)	 
init_str=""

if (( fs_init == 1 )); then
    init_str+="init"
fi
if (( fs_snapshot == 1 )); then
    init_str+=",snapshot"
fi
if (( fs_data_cow == 1 )); then
    init_str+=",data_cow"
fi
if (( fs_wprotect == 1 )); then
    init_str+=",wprotect"
fi



# inserting
echo "umounting..."
sudo umount $MNT_POINT

echo "Removing the old kernel module..."
sudo rmmod nova > /dev/null 2>&1

echo "Inserting the new kernel module..."
sudo insmod nova.ko \
    measure_timing="$(get_modules_options measure_timing)" \
    metadata_csum="$(get_modules_options 'metadata_checksum')" \
    wprotect="$(get_modules_options wprotect)" \
    disable_gc="$(get_modules_options disable_gc)" \
    data_csum="$(get_modules_options data_checksum)"\
    data_parity="$(get_modules_options data_parity)" \
    dram_struct_csum="$(get_modules_options DRAM_checksum)" \
    fragment="$(get_modules_options fragment)" \
    enhance="$(get_modules_options enhance)" \

sleep 1

echo "Mounting..."

# mount
# mount -t NOVA -o init -o wprotect,data_cow /dev/pmem0 $MNT_POINT

sudo mount -t NOVA -o "$init_str" -o dax /dev/pmem0 $MNT_POINT
# sudo mount -t NOVA -o init -o wprotect -o dbgmask=16 /dev/pmem0 $MNT_POINT
#mount -t NOVA -o init /dev/pmem0 $MNT_POINT
echo "Mount with configs: "
echo "$config_json" | jq
echo -e "$CLR_GREEN""> NOVA Mounted!""$CLR_END" 
cd "$ORIGINAL" || exit
