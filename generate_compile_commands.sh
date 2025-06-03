#!/bin/bash

# Absolute path to your project root
PROJECT_ROOT=$(pwd)

# Source directory
SRC_DIR="src"

# Build directory
BUILD_DIR="build"

# Raylib include dirs
RAYLIB_INCLUDE="-Ilibs/raylib/src -Ilibs/raylib/src/external"

# Additional compiler flags (customize if needed)
CFLAGS="-Wall"

# Output file
OUTPUT="compile_commands.json"

# Start JSON array
echo "[" > $OUTPUT

# Find all .c files in src
FILES=($(find $SRC_DIR -name '*.c'))

COUNT=${#FILES[@]}
INDEX=0

for SRC_FILE in "${FILES[@]}"; do
  # Increment index
  INDEX=$((INDEX + 1))

  # Object file path (build/filename.o)
  OBJ_FILE="$BUILD_DIR/$(basename ${SRC_FILE%.*}).o"

  # Compose compile command
  CMD="clang $CFLAGS $RAYLIB_INCLUDE -c $SRC_FILE -o $OBJ_FILE"

  # Write JSON object entry
  echo "  {" >> $OUTPUT
  echo "    \"directory\": \"$PROJECT_ROOT\"," >> $OUTPUT
  echo "    \"command\": \"$CMD\"," >> $OUTPUT
  echo "    \"file\": \"$PROJECT_ROOT/$SRC_FILE\"" >> $OUTPUT
  if [ $INDEX -eq $COUNT ]; then
    echo "  }" >> $OUTPUT
  else
    echo "  }," >> $OUTPUT
  fi
done

# End JSON array
echo "]" >> $OUTPUT

echo "Generated $OUTPUT"

