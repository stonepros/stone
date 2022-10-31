#!/bin/bash -ex

NUM_OSDS=$(stone osd ls | wc -l)
if [ $NUM_OSDS -lt 6 ]; then
    echo "test requires at least 6 OSDs"
    exit 1
fi

NUM_POOLS=$(stone osd pool ls | wc -l)
if [ $NUM_POOLS -gt 0 ]; then
    echo "test requires no preexisting pools"
    exit 1
fi

function wait_for() {
    local sec=$1
    local cmd=$2

    while true ; do
        if bash -c "$cmd" ; then
            break
        fi
        sec=$(( $sec - 1 ))
        if [ $sec -eq 0 ]; then
            echo failed
            return 1
        fi
        sleep 1
    done
    return 0
}

function power2() { echo "x=l($1)/l(2); scale=0; 2^((x+0.5)/1)" | bc -l;}

function eval_actual_expected_val() {
    local actual_value=$1
    local expected_value=$2
    if [[ $actual_value = $expected_value ]]
    then
     echo "Success: " $actual_value "=" $expected_value
    else
      echo "Error: " $actual_value "!=" $expected_value
      exit 1
    fi
}

# enable
stone config set mgr mgr/pg_autoscaler/sleep_interval 60
stone mgr module enable pg_autoscaler
# stone config set global osd_pool_default_pg_autoscale_mode on

# pg_num_min
stone osd pool create meta0 16
stone osd pool create bulk0 16 --bulk
stone osd pool create bulk1 16 --bulk
stone osd pool create bulk2 16 --bulk
stone osd pool set meta0 pg_autoscale_mode on
stone osd pool set bulk0 pg_autoscale_mode on
stone osd pool set bulk1 pg_autoscale_mode on
stone osd pool set bulk2 pg_autoscale_mode on
# set pool size
stone osd pool set meta0 size 2
stone osd pool set bulk0 size 2
stone osd pool set bulk1 size 2
stone osd pool set bulk2 size 2

# get num pools again since we created more pools
NUM_POOLS=$(stone osd pool ls | wc -l)

# get bulk flag of each pool through the command stone osd pool autoscale-status
BULK_FLAG_1=$(stone osd pool autoscale-status | grep 'meta0' | grep -o -m 1 'True\|False' || true)
BULK_FLAG_2=$(stone osd pool autoscale-status | grep 'bulk0' | grep -o -m 1 'True\|False' || true)
BULK_FLAG_3=$(stone osd pool autoscale-status | grep 'bulk1' | grep -o -m 1 'True\|False' || true)
BULK_FLAG_4=$(stone osd pool autoscale-status | grep 'bulk2' | grep -o -m 1 'True\|False' || true)

# evaluate the accuracy of stone osd pool autoscale-status specifically the `BULK` column

eval_actual_expected_val $BULK_FLAG_1 'False'
eval_actual_expected_val $BULK_FLAG_2 'True'
eval_actual_expected_val $BULK_FLAG_3 'True'
eval_actual_expected_val $BULK_FLAG_4 'True'

# This part of this code will now evaluate the accuracy of the autoscaler

# get pool size
POOL_SIZE_1=$(stone osd pool get meta0 size| grep -Eo '[0-9]{1,4}')
POOL_SIZE_2=$(stone osd pool get bulk0 size| grep -Eo '[0-9]{1,4}')
POOL_SIZE_3=$(stone osd pool get bulk1 size| grep -Eo '[0-9]{1,4}')
POOL_SIZE_4=$(stone osd pool get bulk2 size| grep -Eo '[0-9]{1,4}')

# Calculate target pg of each pools
# First Pool is a non-bulk so we do it first.
# Since the Capacity ratio = 0 we first meta pool remains the same pg_num

TARGET_PG_1=$(stone osd pool get meta0 pg_num| grep -Eo '[0-9]{1,4}')
PG_LEFT=$NUM_OSDS*100
NUM_POOLS_LEFT=$NUM_POOLS-1
# Rest of the pool is bulk and even pools so pretty straight forward
# calculations.
TARGET_PG_2=$(power2 $((($PG_LEFT)/($NUM_POOLS_LEFT)/($POOL_SIZE_2))))
TARGET_PG_3=$(power2 $((($PG_LEFT)/($NUM_POOLS_LEFT)/($POOL_SIZE_3))))
TARGET_PG_4=$(power2 $((($PG_LEFT)/($NUM_POOLS_LEFT)/($POOL_SIZE_4))))

# evaluate target_pg against pg num of each pools
wait_for 300 "stone osd pool get meta0 pg_num | grep $TARGET_PG_1"
wait_for 300 "stone osd pool get bulk0 pg_num | grep $TARGET_PG_2"
wait_for 300 "stone osd pool get bulk1 pg_num | grep $TARGET_PG_3"
wait_for 300 "stone osd pool get bulk2 pg_num | grep $TARGET_PG_4"

# target ratio
stone osd pool set meta0 target_size_ratio 5
stone osd pool set bulk0 target_size_ratio 1
sleep 60
APGS=$(stone osd dump -f json-pretty | jq '.pools[0].pg_num_target')
BPGS=$(stone osd dump -f json-pretty | jq '.pools[1].pg_num_target')
test $APGS -gt 100
test $BPGS -gt 10

# small ratio change does not change pg_num
stone osd pool set meta0 target_size_ratio 7
stone osd pool set bulk0 target_size_ratio 2
sleep 60
APGS2=$(stone osd dump -f json-pretty | jq '.pools[0].pg_num_target')
BPGS2=$(stone osd dump -f json-pretty | jq '.pools[1].pg_num_target')
test $APGS -eq $APGS2
test $BPGS -eq $BPGS2

# target_size
stone osd pool set meta0 target_size_bytes 1000000000000000
stone osd pool set bulk0 target_size_bytes 1000000000000000
stone osd pool set meta0 target_size_ratio 0
stone osd pool set bulk0 target_size_ratio 0
wait_for 60 "stone health detail | grep POOL_TARGET_SIZE_BYTES_OVERCOMMITTED"

stone osd pool set meta0 target_size_bytes 1000
stone osd pool set bulk0 target_size_bytes 1000
stone osd pool set meta0 target_size_ratio 1
wait_for 60 "stone health detail | grep POOL_HAS_TARGET_SIZE_BYTES_AND_RATIO"

stone osd pool rm meta0 meta0 --yes-i-really-really-mean-it
stone osd pool rm bulk0 bulk0 --yes-i-really-really-mean-it
stone osd pool rm bulk1 bulk1 --yes-i-really-really-mean-it
stone osd pool rm bulk2 bulk2 --yes-i-really-really-mean-it

echo OK

