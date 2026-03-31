#!/bin/bash

# filepath: /Users/leviladen/cse130/lladen/asgn1/test_scripts/set_large_bin.sh

# Function to clean up generated files
cleanup() {
    rm -f large_binary_file.bin large_binary_file_read_back.bin
}

# Generate a large binary file (10 MB)
dd if=/dev/urandom of=large_binary_file.bin bs=1M count=10

# Use the set command to write the file's contents
echo -ne "set\nlarge_binary_file.bin\n$(wc -c < large_binary_file.bin)\n" | cat - large_binary_file.bin | ./memory

# Use the get command to read the file's contents back
echo -ne "get\nlarge_binary_file.bin\n" | ./memory > large_binary_file_read_back.bin

# Compare the original file with the read-back file
if cmp -s large_binary_file.bin large_binary_file_read_back.bin; then
    echo "Test passed: The files are identical."
    rc=0
else
    echo "Test failed: The files are different."
    rc=1
fi

# Clean up
cleanup
exit $rc