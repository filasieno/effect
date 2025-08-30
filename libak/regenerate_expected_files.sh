#!/bin/bash

# Script to regenerate all expected test files from the actual test outputs
# This should be run after verifying that the parser is working correctly

echo "Regenerating all expected test files from actual outputs..."

for output_file in $(find build/test_output/json -name "output.txt"); do
    # Extract test name from path
    dir_name=$(basename "$(dirname "$output_file")")
    test_name=${dir_name%.txt}

    expected_file="test/json/data/${test_name}_exp.txt"

    if [[ -f "$expected_file" ]]; then
        echo "Updating $expected_file"
        cp "$output_file" "$expected_file"
    else
        echo "Warning: Expected file $expected_file not found for $output_file"
    fi
done

echo "Finished regenerating expected files."
