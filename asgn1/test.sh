#!/bin/bash

# Generate a file with 10 million 'A's as input
echo "Generating test input file..."
python3 -c "print('A' * 10000000, end='')" > large_input.txt

# Run the program with the generated input
echo "Running test with large file content..."
echo -e "set\nlarge_file_test.txt\n10000000\n$(cat large_input.txt)" | ./memory > output.txt 2> error.txt

# Verify results
RESULT=$?
EXPECTED_SIZE=10000000

echo "Checking results..."
if [[ $RESULT -eq 0 ]]; then
    echo "Program exited with return code 0: PASS"
else
    echo "Program exited with non-zero return code: FAIL"
    cat error.txt
    exit 1
fi

if [[ -f "large_file_test.txt" ]]; then
    ACTUAL_SIZE=$(stat --format="%s" large_file_test.txt)
    if [[ $ACTUAL_SIZE -eq $EXPECTED_SIZE ]]; then
        echo "File size is correct ($ACTUAL_SIZE bytes): PASS"
    else
        echo "File size is incorrect ($ACTUAL_SIZE bytes): FAIL"
    fi
else
    echo "Output file not created: FAIL"
    exit 1
fi

# Validate file content (optional, could be slow for very large files)
echo "Validating file content..."
head -c 10 large_file_test.txt | grep -q "^AAAAAAAAAA$"
if [[ $? -eq 0 ]]; then
    echo "File content validation: PASS"
else
    echo "File content validation: FAIL"
fi

echo "Test complete."
