output_file="performance-table"
echo "workload size bandwidth fast_gc thorough_gc write" > "$output_file"

WORKLOADS=("sw" "rw" "sow" "row")
SIZES=("1G" "2G" "4G" "8G" "16G" "32G" "64G" "128G")

for workload in "${WORKLOADS[@]}"; do
    for size in "${SIZES[@]}"; do
        fio_file="DATA/$workload/fio_output_${size}"
        bandwidth=$(grep -oP 'BW=\K[0-9]+' "$fio_file")

        dmesg_file="DATA/$workload/dmesg_output_${size}"

        fast_gc_time=$(grep -oP 'nova: log_fast_gc: count \d+, timing \K\d+' "$dmesg_file" | head -1)
        thorough_gc_time=$(grep -oP 'nova: log_thorough_gc: count \d+, timing \K\d+' "$dmesg_file" | head -1)
        cow_write_time=$(grep -oP 'nova: do_cow_write: count \d+, timing \K\d+' "$dmesg_file" | head -1)

        bandwidth=${bandwidth:-0}
        fast_gc_time=${fast_gc_time:-0}
        thorough_gc_time=${thorough_gc_time:-0}
        cow_write_time=${cow_write_time:-0}

        echo "$workload $size $bandwidth $fast_gc_time $thorough_gc_time $cow_write_time" >> "$output_file"
    done
done