#!/usr/bin/env bash

servers=1
workload=workloads/basic_test.toml
cachesize=0
cachealg=FIFO

./test_scripts/cachetest.sh $servers $workload $cachesize $cachealg
exit $?
