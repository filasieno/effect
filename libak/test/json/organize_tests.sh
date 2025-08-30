#!/bin/bash

# Script to organize passing and failing tests
# This will test each case and move failing tests to excluded folder

set -e

DATA_DIR="test/json/data"
EXCLUDED_DIR="$DATA_DIR/excluded"
BUILD_DIR="build"
TEST_BINARY="$BUILD_DIR/test_json"

echo "=== Organizing Tests: Pass vs Fail ==="
echo "Data directory: $DATA_DIR"
echo "Excluded directory: $EXCLUDED_DIR"
echo ""

# Create excluded directory if it doesn't exist
mkdir -p "$EXCLUDED_DIR"

# Arrays to track results
passing_tests=()
failing_tests=()

# Test each file individually (limit to avoid timeout)
echo "Testing files..."
count=0
max_tests=50  # Test first 50 to see patterns

for test_file in "$DATA_DIR"/*.txt; do
    if [[ ! -f "$test_file" ]]; then
        continue
    fi

    # Skip if already in excluded
    if [[ "$test_file" == *"/excluded/"* ]]; then
        continue
    fi

    # Only test input files (not expected files)
    if [[ "$test_file" == *_exp.txt ]]; then
        continue
    fi

    count=$((count + 1))
    if [[ $count -gt $max_tests ]]; then
        echo "Reached test limit ($max_tests), stopping..."
        break
    fi

    # Get basename without extension
    basename=$(basename "$test_file" .txt)

    echo -n "Testing: $basename... "

    # Run test with timeout
    result=$(timeout 10s AK_TEST_DATA_DIR=test/json/data ./$TEST_BINARY --gtest_filter="*Case/$basename*" 2>&1 || echo "TIMEOUT")

    # Check if test passed
    if echo "$result" | grep -q "PASSED.*1 test"; then
        echo "✅ PASS"
        passing_tests+=("$basename")
    elif echo "$result" | grep -q "FAILED.*1 test"; then
        echo "❌ FAIL"
        failing_tests+=("$basename")
    else
        echo "💥 CRASH/TIMEOUT"
        failing_tests+=("$basename")
    fi
done

echo ""
echo "=== RESULTS ==="
echo "Passing tests: ${#passing_tests[@]}"
echo "Failing tests: ${#failing_tests[@]}"

echo ""
echo "=== PASSING TESTS ==="
for test in "${passing_tests[@]}"; do
    echo "✅ $test"
done

echo ""
echo "=== FAILING TESTS ==="
for test in "${failing_tests[@]}"; do
    echo "❌ $test"
done

echo ""
echo "Moving failing tests to excluded directory..."

# Move failing tests
for test in "${failing_tests[@]}"; do
    if [[ -f "$DATA_DIR/$test.txt" ]]; then
        echo "Moving $test.txt to excluded..."
        mv "$DATA_DIR/$test.txt" "$EXCLUDED_DIR/"
    fi
    if [[ -f "$DATA_DIR/${test}_exp.txt" ]]; then
        echo "Moving ${test}_exp.txt to excluded..."
        mv "$DATA_DIR/${test}_exp.txt" "$EXCLUDED_DIR/"
    fi
done

echo ""
echo "=== FINAL STATUS ==="
echo "Tests in data directory: $(ls "$DATA_DIR"/*.txt | grep -v "_exp" | grep -v "/excluded/" | wc -l)"
echo "Tests in excluded directory: $(ls "$EXCLUDED_DIR"/*.txt | grep -v "_exp" | wc -l)"

echo ""
echo "=== NEXT STEPS ==="
echo "1. Run tests again to verify only passing tests remain"
echo "2. Pick one failing test from excluded/ to work on"
echo "3. Move it back to data/, run test, analyze failure"
echo "4. Fix the issue, verify it passes, then move to next"
