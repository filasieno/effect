#!/bin/bash

# Script to fix multi-buffer test cases that are missing PARSER_STOPPED and STATE_DONE events

echo "Finding multi-buffer test outputs that need PARSER_STOPPED and STATE_DONE..."

for output_file in $(find build/test_output/json -name "output.txt" -exec grep -l "result=CONTINUE" {} \;); do
    # Extract test name from path (remove .txt extension if present)
    dir_name=$(basename "$(dirname "$output_file")")
    test_name=${dir_name%.txt}

    expected_file="test/json/data/${test_name}_exp.txt"

    if [[ -f "$expected_file" ]]; then
        echo "Checking $test_name..."

        # Check if the expected file has multiple buffers (contains ---)
        buffer_count=$(grep -c "^---$" "$expected_file")
        if [[ $buffer_count -gt 1 ]]; then
            # Check if it already has PARSER_STOPPED
            if ! grep -q "PARSER_STOPPED" "$expected_file"; then
                echo "  Fixing $test_name - adding PARSER_STOPPED and STATE_DONE"

                # Find the last EVENT section and add the missing events
                # This is a bit tricky, so let's read the file and modify it
                content=$(cat "$expected_file")

                # Check if the last section is empty (ends with ---EVENT\n)
                if echo "$content" | tail -2 | grep -q "^---$"; then
                    if echo "$content" | tail -1 | grep -q "^EVENT$"; then
                        # Replace the empty EVENT section with the proper events
                        new_content=$(echo "$content" | sed '$s/EVENT$/EVENT\nPARSER_STOPPED\nSTATE_DONE/')
                        echo "$new_content" > "$expected_file"
                    fi
                fi
            else
                echo "  $test_name already has PARSER_STOPPED"
            fi
        fi
    fi
done

echo "Done fixing multi-buffer test cases."
