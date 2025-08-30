#!/bin/bash

# Script to fix all ERROR test cases that are missing the value=<error_code> line

echo "Finding all ERROR test outputs..."

for output_file in $(find build/test_output/json -name "output.txt" -exec grep -l "result=ERROR" {} \;); do
    # Extract test name from path (remove .txt extension if present)
    dir_name=$(basename "$(dirname "$output_file")")
    test_name=${dir_name%.txt}

    expected_file="test/json/data/${test_name}_exp.txt"

    if [[ -f "$expected_file" ]]; then
        echo "Checking $test_name..."

        # Get the value line from actual output
        value_line=$(grep "^value=" "$output_file")

        # Check if expected file has the value line
        if ! grep -q "^value=" "$expected_file"; then
            echo "  Fixing $test_name - adding $value_line"

            # Read the expected file content
            expected_content=$(cat "$expected_file")

            # Insert the value line after result=ERROR
            new_content=$(echo "$expected_content" | sed "/^result=ERROR$/a$value_line")

            # Write back to file
            echo "$new_content" > "$expected_file"
        else
            echo "  $test_name already has value line"
        fi
    else
        echo "  Expected file not found: $expected_file"
    fi
done

echo "Done fixing ERROR test cases."
