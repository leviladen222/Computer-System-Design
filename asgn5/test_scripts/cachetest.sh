#!/usr/bin/env bash

servers=$1
workload=$2
cachesize=$3
cachealg=$4

log=rosesays.csv
outdir=out
replaydir=replay

echo "rose_wrapper start:"
./test_scripts/rose_wrapper.sh $servers $workload $log $outdir $cachealg $cachesize
rc=$?

echo "rose end. rc=$rc"
if [[ $rc -ne 0 ]]; then
    echo "rose_wrapper failed!?"
    exit 1
fi

# Check the output. You might do a few things, depending on the test:

### Use this to check the message bodies
echo "coroner.py start:"
./test_scripts/coroner.py -r $replaydir -d $outdir $workload 
rc=$?
echo "coroner end. rc=$rc"

if [[ $rc -ne 0 ]]; then
    echo "Coroner failed!"
    exit 1
fi

if [[ $cachesize -gt 0 ]]; then
    echo "methodman.py start:"
    ### Use this if you need to check for caching
    # Not necesssary because this is a passthrough tests.
    ./test_scripts/methodman.py -a $cachealg -s $cachesize $log 
    mrc=$?
    echo "methodman end. rc=$mrc"

    if [[ $mrc -ne 0 ]]; then
        echo "methodman failed!"
        exit 1
    fi  
fi
exit 0
