#!/usr/bin/env bash

servers=2
workload=workloads/multihost_stress.toml
cachesize=3
cachealg=FIFO

./test_scripts/cachetest.sh $servers $workload $cachesize $cachealg
rc=$?

exit $rc