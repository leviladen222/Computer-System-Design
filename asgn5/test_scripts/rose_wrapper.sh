#!/usr/bin/env bash

source "test_scripts/utils.sh"

servers=$1
workload=$2
log=$3
outdir=$4
cachealg=$5
cachesize=$6

if [[ `check_dir` -eq 1 ]]; then
    exit 1
fi

# start the servers
serverpids=""
hosts=""
for i in $(seq 0 $(($servers - 1))); do 
    server=($(start_server))
    serverpid=${server[0]}
    serverport=${server[1]}

    host="[[events]]\ntype = \"HOST\"\nid = $i\nhostname = \"127.0.0.1\"\nport = $serverport\n\n"
    hosts="${hosts}${host}"

    ## Track all of these
    serverpids="$serverpids $serverpid"
done

cat <(printf "$hosts") $workload > new_workload.toml
#cat new_workload.toml

# start the proxy process
proxyport=`get_port`
if [[ proxyport -eq 1 ]]; then
    exit 1
fi
./httpproxy "$proxyport" $cachealg $cachesize >output.txt 2>error.txt &
pid=$!

# Wait until we can connect.
wait_for_listen $proxyport
wait_rc=$?

if [[ $wait_rc -eq 1 ]]; then
    echo "Proxy didn't listen on $port in time"
    kill -9 $pid
    wait $pid &>/dev/null
    exit 1
fi

./test_scripts/rosemaylie.py -o 127.0.0.1 -p $proxyport -d $outdir new_workload.toml > $log 2>rose-error.txt

## Wait for the proxy to die.
kill $pid
wait $pid &>/dev/null

## wait for the servers to die.
for pid in $serverpids; do
    echo "killing and waiting on $pid"
    kill -9 $pid
    wait $pid &>/dev/null
done

exit 0



