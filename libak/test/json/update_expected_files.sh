#!/bin/bash

# Script to update expected test files to match new serialization format
# Changes:
# 1. Add STATE_INITIALIZED after EVENT line
# 2. Add STATE_DONE before the last line (if result=DONE)
# 3. Add STATE_ERROR <code> for error cases
# 4. Add double quotes around string values in STRING, KEY, and STRING_CHARS events

set -e

DATA_DIR="test/json/data"
OUTPUT_DIR="build/test_output/json"

echo "Updating expected files to match new serialization format..."

# Find all .txt files that are expected output files (ending with _exp.txt)
find "$DATA_DIR" -name "*_exp.txt" | while read -r exp_file; do
    echo "Processing $exp_file"

    # Get the corresponding output file
    base_name=$(basename "$exp_file" "_exp.txt")
    output_file="$OUTPUT_DIR/${base_name}.txt/output.txt"

    if [[ -f "$output_file" ]]; then
        echo "  Using actual output from $output_file"

        # Create a temporary updated version
        temp_file=$(mktemp)

        # Read the actual output and format it properly
        sed -n '/^EVENT$/,$p' "$output_file" | tail -n +2 > "$temp_file"

        # Update the expected file
        # First, copy the result line
        head -1 "$output_file" > "${exp_file}.new"
        echo "---" >> "${exp_file}.new"
        echo "EVENT" >> "${exp_file}.new"
        echo "STATE_INITIALIZED" >> "${exp_file}.new"

        # Add the events from the actual output (excluding STATE_INITIALIZED and STATE_DONE if present)
        grep -v "^STATE_INITIALIZED$" "$temp_file" | grep -v "^STATE_DONE$" | grep -v "^PARSER_STOPPED$" >> "${exp_file}.new"

        # Add final state if it's DONE
        if head -1 "$output_file" | grep -q "result=DONE"; then
            echo "STATE_DONE" >> "${exp_file}.new"
        fi

        # Replace the original file
        mv "${exp_file}.new" "$exp_file"

        rm "$temp_file"
    else
        echo "  Warning: No corresponding output file found for $exp_file"
    fi
done

echo "Expected files updated successfully!"
