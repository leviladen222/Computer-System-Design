#!/usr/bin/env bash

helpers=("test_scripts/utils.sh" "test_scripts/cachetest.sh" "test_scripts/rose_wrapper.sh")

for test in `ls test_scripts/*.sh`; do
	if [[ ! " ${helpers[@]} " =~ " $test " ]]; then
	    echo "------------------------------------------------------------------------"
	    printf "$test:\n"
	    echo "------------------------------------------------------------------------"

	    output=$(./$test)
	    return_code=$?
	    if [ $return_code -eq 0 ]; then
		printf "SUCCESS\n\n"
	    else
		printf "FAILED\n\n"
		printf "Return code: $return_code\n"
		printf "Output:\n$output\n\n"
		fi
	fi
done
