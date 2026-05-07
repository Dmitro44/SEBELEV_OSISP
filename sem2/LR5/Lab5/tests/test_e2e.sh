#!/usr/bin/env bash

set -euo pipefail

echo "Running setup_mock_fs.sh..."
bash tests/setup_mock_fs.sh

echo "Running analyzer tests/mock_fs 2 > tests/results.txt"
./analyzer tests/mock_fs 2 > tests/results.txt

echo "Checking output format..."
# Check that the output format matches <pathname>-<signature>-<timestamp>-<agent_pid>
# Allow paths with `-` or `_`, basic check for numeric signature, timestamp, pid.
while read -r line; do
    if [[ ! "$line" =~ ^(.*)-([0-9]+)-([0-9]+)-([0-9]+)$ ]]; then
        echo "Error: Output format failed on line: $line"
        exit 1
    fi
done < tests/results.txt

echo "Checking for zombie processes..."
# Check if there are any zombie processes left over
ZOMBIES=$(ps aux | awk '$8 ~ /Z/ && $11 ~ /analyzer/' || true)
if [ -n "$ZOMBIES" ]; then
    echo "Error: Zombie processes found:"
    echo "$ZOMBIES"
    exit 1
fi

echo "Verifying number of files in output..."
LINE_COUNT=$(wc -l < tests/results.txt)
if [ "$LINE_COUNT" -ne 6 ]; then
    echo "Error: Expected 6 lines, got $LINE_COUNT"
    exit 1
fi

echo "Verifying specific signatures..."
# A -> 65
if ! grep -q "tests/mock_fs/dir1/file1.txt-65-" tests/results.txt; then
    echo "Error: Failed to find signature 65 for file1.txt"
    exit 1
fi

# BC -> 1
if ! grep -q "tests/mock_fs/dir1/file2.txt-1-" tests/results.txt; then
    echo "Error: Failed to find signature 1 for file2.txt"
    exit 1
fi

# hello -> 98
if ! grep -q "tests/mock_fs/dir2/subdir1/file3.txt-98-" tests/results.txt; then
    echo "Error: Failed to find signature 98 for file3.txt"
    exit 1
fi

# world -> 98
if ! grep -q "tests/mock_fs/dir2/file4.txt-98-" tests/results.txt; then
    echo "Error: Failed to find signature 98 for file4.txt"
    exit 1
fi

# XYZ -> 91
if ! grep -q "tests/mock_fs/dir3/nested/deeper/file5.txt-91-" tests/results.txt; then
    echo "Error: Failed to find signature 91 for file5.txt"
    exit 1
fi

# test data -> 38
if ! grep -q "tests/mock_fs/root_file.txt-38-" tests/results.txt; then
    echo "Error: Failed to find signature 38 for root_file.txt"
    exit 1
fi

echo "E2E verification passed successfully!"
