#!/bin/bash

# Comprehensive script to update all expected test files to match the new serialization format
# Changes made:
# 1. Add STATE_INITIALIZED after EVENT line
# 2. Add STATE_DONE for successful parsing (result=DONE)
# 3. Add STATE_ERROR <code> for error cases (result=ERROR)
# 4. Add value=<error_code> for ERROR cases
# 5. Add PARSER_STOPPED for multi-buffer tests
# 6. Add double quotes around string values in STRING, KEY, KEY_CHARS, STRING_CHARS events

echo "Updating all expected test files to match new serialization format..."

for exp_file in $(find test/json/data -name "*_exp.txt" | grep -v excluded); do
    echo "Processing $exp_file..."

    # Get the corresponding input file to determine the test case
    input_file="${exp_file%_exp.txt}.txt"

    if [[ ! -f "$input_file" ]]; then
        echo "  Warning: Input file $input_file not found, skipping"
        continue
    fi

    # Read the expected content
    content=$(cat "$exp_file")

    # Check if this is a multi-buffer test (contains ---)
    buffer_count=$(echo "$content" | grep -c "^---$")

    # Get the result type
    result_type=$(echo "$content" | grep "^result=" | head -1 | cut -d'=' -f2)

    # Update the content based on the patterns
    new_content="$content"

    # 1. Add STATE_INITIALIZED if not present and EVENT is present
    if echo "$new_content" | grep -q "^EVENT$" && ! echo "$new_content" | grep -q "STATE_INITIALIZED"; then
        new_content=$(echo "$new_content" | sed 's/^EVENT$/EVENT\nSTATE_INITIALIZED/')
    fi

    # 2. Add final state events
    if [[ "$result_type" == "DONE" ]]; then
        # For successful parsing, add STATE_DONE if not present
        if ! echo "$new_content" | grep -q "STATE_DONE"; then
            if [[ $buffer_count -gt 1 ]]; then
                # Multi-buffer: add PARSER_STOPPED and STATE_DONE in the last section
                new_content=$(echo "$new_content" | sed '$s/$/\nPARSER_STOPPED\nSTATE_DONE/')
            else
                # Single buffer: add STATE_DONE at the end
                new_content=$(echo "$new_content" | sed '$s/$/\nSTATE_DONE/')
            fi
        fi
    elif [[ "$result_type" == "ERROR" ]]; then
        # For error cases, ensure value= is present and add STATE_ERROR if not present
        if ! echo "$new_content" | grep -q "^value="; then
            # We need to determine the error code - this is tricky without running the test
            # For now, add a placeholder that will need manual fixing
            new_content=$(echo "$new_content" | sed 's/^result=ERROR$/result=ERROR\nvalue=UNKNOWN/')
        fi
        if ! echo "$new_content" | grep -q "STATE_ERROR"; then
            # Add STATE_ERROR with the error code
            error_code=$(echo "$new_content" | grep "^value=" | cut -d'=' -f2)
            if [[ -n "$error_code" && "$error_code" != "UNKNOWN" ]]; then
                new_content=$(echo "$new_content" | sed "s/\$/\nSTATE_ERROR $error_code/")
            fi
        fi
    fi

    # 3. Add double quotes around string values (basic pattern)
    # This is complex and might need manual adjustment for edge cases
    new_content=$(echo "$new_content" | sed 's/STRING \([^"]\)/STRING "\1"/g')
    new_content=$(echo "$new_content" | sed 's/KEY \([^"]\)/KEY "\1"/g')
    new_content=$(echo "$new_content" | sed 's/KEY_CHARS \([^"]\)/KEY_CHARS "\1"/g')
    new_content=$(echo "$new_content" | sed 's/STRING_CHARS \([^"]\)/STRING_CHARS "\1"/g')

    # Write back if changed
    if [[ "$new_content" != "$content" ]]; then
        echo "$new_content" > "$exp_file"
        echo "  Updated $exp_file"
    else
        echo "  No changes needed for $exp_file"
    fi
done

echo "Finished updating expected files."
echo "Note: Some ERROR cases may need manual verification of error codes."
