#!/bin/bash

MOUNT_PATH="/home/alex/workspace/fs/src/mount1"
FILENAME="$MOUNT_PATH/testfile1"
OUTPUT="readwritepercent_results.log"

echo "read_write_percent,bandwidth_read,iops_read,bandwidth_write,iops_write,latency_read,latency_write" > "$OUTPUT"
for read_write_percent in 90 70 50 30 10; do
    echo "Running test with read_write_percent=$read_write_percent..."

    fio --name=read-write-test \
        --rw=randrw \
        --rwmixread=$read_write_percent \
        --bs=4k \
        --randseed=1234 \
        --size=10M \
        --ioengine=libaio \
        --direct=0 \
        --numjobs=1 \
        --runtime=30 \
        --group_reporting \
        --filename=$FILENAME > out.tmp

    BW_READ=$(grep -Po 'READ:.*?bw=\K[0-9.]+' out.tmp)
    BW_WRITE=$(grep -Po 'WRITE:.*?bw=\K[0-9.]+' out.tmp)
    IOPS_READ=$(grep -Po 'read:.*?IOPS=\K[0-9.]+' out.tmp)
    IOPS_WRITE=$(grep -Po 'write:.*?IOPS=\K[0-9.]+' out.tmp)

    LAT_READ=$(grep -A3 'read:.*' out.tmp | grep -Po 'clat.*?avg=\K[0-9.]+' )
    LAT_WRITE=$(grep -A3 'write:.*' out.tmp | grep -Po 'clat.*?avg=\K[0-9.]+' )



    echo "$read_write_percent,$BW_READ,$IOPS_READ,$BW_WRITE,$IOPS_WRITE,$LAT_READ,$LAT_WRITE" >> "$OUTPUT"
done









