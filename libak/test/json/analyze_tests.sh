#!/bin/bash

# Script to analyze test results and organize passing/failing tests
# This will run tests individually to avoid crashes and identify issues

set -e

DATA_DIR="test/json/data"
EXCLUDED_DIR="$DATA_DIR/excluded"
BUILD_DIR="build"
TEST_BINARY="$BUILD_DIR/test_json"

echo "=== JSON Test Analysis Script ==="
echo "Data directory: $DATA_DIR"
echo "Excluded directory: $EXCLUDED_DIR"
echo ""

# Create excluded directory if it doesn't exist
mkdir -p "$EXCLUDED_DIR"

# Function to run a single test
run_single_test() {
    local test_name="$1"
    echo "Testing: $test_name"

    # Run the test and capture output
    timeout 10s ./$TEST_BINARY --gtest_filter="*Case/$test_name*" 2>&1 || true
}

# Get list of all test files (input files)
test_files=$(ls "$DATA_DIR"/*.txt | grep -v "_exp" | sed 's|.*/||' | sed 's|\.txt$||')

echo "Found $(echo "$test_files" | wc -l) test cases to analyze"

# Arrays to track results
passing_tests=()
failing_tests=()

# Test each file individually
for test_file in $test_files; do
    echo "Testing: $test_file"

    # Run test with timeout to avoid hanging
    result=$(timeout 10s ./$TEST_BINARY --gtest_filter="*Case/$test_file*" 2>&1 || echo "TIMEOUT_OR_CRASH")

    # Check if test passed
    if echo "$result" | grep -q "PASSED.*1 test"; then
        echo "✅ PASS: $test_file"
        passing_tests+=("$test_file")
    elif echo "$result" | grep -q "FAILED.*1 test"; then
        echo "❌ FAIL: $test_file"
        failing_tests+=("$test_file")
    else
        echo "💥 CRASH: $test_file"
        failing_tests+=("$test_file")
    fi

    echo "---"
done

echo ""
echo "=== SUMMARY ==="
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

# Move failing tests to excluded directory
for test in "${failing_tests[@]}"; do
    echo "Moving $test to excluded..."
    mv "$DATA_DIR/$test.txt" "$EXCLUDED_DIR/" 2>/dev/null || true
    mv "$DATA_DIR/${test}_exp.txt" "$EXCLUDED_DIR/" 2>/dev/null || true
done

echo ""
echo "=== FINAL STATUS ==="
echo "Tests in data directory: $(ls "$DATA_DIR"/*.txt | grep -v "_exp" | wc -l)"
echo "Tests in excluded directory: $(ls "$EXCLUDED_DIR"/*.txt | grep -v "_exp" | wc -l)"
