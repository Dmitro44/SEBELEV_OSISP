#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="tests/mock_fs"

rm -rf "$ROOT_DIR"
mkdir -p "$ROOT_DIR/dir1" "$ROOT_DIR/dir2/subdir1" "$ROOT_DIR/dir3/nested/deeper"

printf 'A' > "$ROOT_DIR/dir1/file1.txt"
printf 'BC' > "$ROOT_DIR/dir1/file2.txt"
printf 'hello' > "$ROOT_DIR/dir2/subdir1/file3.txt"
printf 'world' > "$ROOT_DIR/dir2/file4.txt"
printf 'XYZ' > "$ROOT_DIR/dir3/nested/deeper/file5.txt"
printf 'test data' > "$ROOT_DIR/root_file.txt"
