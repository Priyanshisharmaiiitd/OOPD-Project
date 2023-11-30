#!/bin/bash

# Function to create files in a directory
create_files() {
  local size=$1
  local count=$2
  local directory=$3

  mkdir -p "$directory"
  for ((i = 1; i <= count; i++)); do
    dd if=/dev/zero of="$directory/file_$i" bs="$size" count=1 status=none &
  done
  wait
}

# Function to create files and subdirectories recursively
create_recursive_files() {
  local size=$1
  local count=$2
  local directory=$3
  local depth=$4

  if [[ $depth -eq 0 ]]; then
    return
  fi

  mkdir -p "$directory"
  for ((i = 1; i <= count; i++)); do
    dd if=/dev/zero of="$directory/file_$i" bs="$size" count=1 status=none &
  done

  for ((i = 1; i <= 10; i++)); do
    create_recursive_files "$size" "$count" "$directory/subdirectory_$i" "$((depth - 1))"
  done
  wait
}

# Measure time for Case 1: 100 files of 1KB each in directory1
time create_files 1K 100 directory1

# Measure time for Case 2: 10,000 files of 1KB each in directory2
time create_files 1K 10000 directory2

# Measure time for Case 3: 100 files of 1KB each with recursive subdirectories until 10,000 files
time create_recursive_files 1K 100 directory3 100

# Count the total number of files in directory3
total_files=$(find directory3 -type f | wc -l)
echo "Total files created in directory3: $total_files"