#!/bin/bash

# filepath: /Users/leviladen/cse130/lladen/asgn1/test_scripts/set_frankenstein.sh

# Function to clean up generated files
cleanup() {
    rm -f new_frankenstein.txt
}

# Read the contents of frankenstein.txt
contents=$(cat frankenstein.txt)
content_length=$(wc -c < frankenstein.txt)

# Use the set command to write the contents to a new file
echo -ne "set\nnew_frankenstein.txt\n$content_length\n$contents\n" | ./memory

# Verify that the new file contains the correct contents
if cmp -s frankenstein.txt new_frankenstein.txt; then
    echo "Test passed: The files are identical."
    rc=0
else
    echo "Test failed: The files are different."
    rc=1
fi

# Clean up
exit $rc