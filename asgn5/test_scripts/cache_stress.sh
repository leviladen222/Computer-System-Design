#!/usr/bin/env bash

servers=2
workload=workloads/multihost_stress.toml
cachesize=1024
cachealg=FIFO

./test_scripts/cachetest.sh $servers $workload $cachesize $cachealg
rc=$?

exit $rc