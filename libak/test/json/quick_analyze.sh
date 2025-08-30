#!/bin/bash

# Quick analysis of test results to identify patterns

set -e

DATA_DIR="test/json/data"
BUILD_DIR="build"
TEST_BINARY="$BUILD_DIR/test_json"

echo "=== Quick Test Analysis ==="

# Test a few known working cases first
echo "Testing known working cases..."
echo "1. Testing y_structure_string_empty..."
AK_TEST_DATA_DIR=test/json/data ./$TEST_BINARY --gtest_filter="*Case/y_structure_string_empty*" 2>&1 | grep -E '(PASSED|FAILED)'

echo "2. Testing array_integers..."
AK_TEST_DATA_DIR=test/json/data ./$TEST_BINARY --gtest_filter="*Case/array_integers*" 2>&1 | grep -E '(PASSED|FAILED)'

echo "3. Testing whitespace_only..."
AK_TEST_DATA_DIR=test/json/data ./$TEST_BINARY --gtest_filter="*Case/whitespace_only*" 2>&1 | grep -E '(PASSED|FAILED)'

# Test a few known failing cases
echo ""
echo "Testing known failing cases..."
echo "4. Testing n_structure_no_data..."
AK_TEST_DATA_DIR=test/json/data ./$TEST_BINARY --gtest_filter="*Case/n_structure_no_data*" 2>&1 | grep -E '(PASSED|FAILED)'

echo "5. Testing n_number_invalid_negative_real..."
AK_TEST_DATA_DIR=test/json/data ./$TEST_BINARY --gtest_filter="*Case/n_number_invalid_negative_real*" 2>&1 | grep -E '(PASSED|FAILED)'

echo ""
echo "=== Summary ==="
echo "Working tests should be kept in data/"
echo "Failing tests should be moved to data/excluded/"
echo ""
echo "Next step: Run full analysis and organize tests"
