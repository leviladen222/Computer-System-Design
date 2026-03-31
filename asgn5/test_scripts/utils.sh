cleanup() {
    for file in $@; do
	rm -r $file
    done
}

check_dir() {
    if [ ! -f httpproxy ]; then
	echo "I cannot find httpproxy binary.  Did you..."
	echo "(1) build your assignment?"
	echo "(2) run this script from your assignment directory?"
	return 1
    fi
    return 0
}



get_port() {
    #### usable port range 32768 -> 65535

    ss --tcp state CLOSE-WAIT --kill &>/dev/null
    ss --tcp state TIME-WAIT --kill &>/dev/null
    getports=`netstat -antu | tail -n +3 | awk '{split($4, parts,":"); print parts[length(parts)]}' | uniq`

    declare -A portmap
    for port in $getports; do
	portmap[$port]=1
    done

    return_port=32768
    while [[ $return_port -le 65535 ]]; do
	if [[ ! -v portmap[$return_port] ]]; then
            echo $return_port
            exit 0
	fi
	((return_port+=1))
    done

    echo "couldn't find port"
    exit 1
}


wait_for_listen() {
    port=$1
    count=1

    while [[ `ss -tlH sport = :$port | wc -l` -lt 1 && "$count" -lt 100 ]]; do
	sleep .05
	count=$(($count + 1))
    done

    if [[ "$count" -eq 40 ]]; then
	return 1
    fi

    return 0
}



start_server() { 
    port=`get_port`
    if [[ port -eq 1 ]]; then
        exit 1
    fi

    # Start up server.
    ./test_scripts/httpserver -t 4 $port > serveroutput.txt 2>servererror.txt &
    pid=$!

    # Wait until we can connect.
    wait_for_listen $port
    wait_rc=$?

    if [[ $wait_rc -eq 1 ]]; then
        echo "Server didn't listen on $port in time"
        kill -9 $pid
        wait $pid &>/dev/null
        exit 1
    fi

    # "return" two values, pid for server and the port.
    local result=("$pid" "$port")
    echo "${result[@]}"
}