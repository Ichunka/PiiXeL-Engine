#!/usr/bin/env bash
set -euo pipefail

# ---------- paths ----------
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK_DIR="$PROJECT_ROOT/games/MyFirstGame"

# ---------- utils ----------
jobs() {
  if command -v nproc >/dev/null 2>&1; then nproc
  elif [[ "$(uname)" == "Darwin" ]] && command -v sysctl >/dev/null 2>&1; then sysctl -n hw.ncpu
  else echo 4
  fi
}

cache_get() {
  local cache="$1" key="$2"
  [[ -f "$cache" ]] || return 1
  # CMakeCache format: <key>:<type>=<value>
  awk -F'[:=]' -v k="$key" '$1==k{print $3}' "$cache"
}

need_configure() {
  local bdir="$1" build_type="$2" build_editor="$3" cc="$4" cxx="$5" gen="$6"
  local cache="$bdir/CMakeCache.txt"
  [[ ! -f "$cache" ]] && return 0
  [[ "$(cache_get "$cache" CMAKE_BUILD_TYPE || true)" != "$build_type" ]] && return 0
  [[ "$(cache_get "$cache" BUILD_EDITOR || true)" != "$build_editor" ]] && return 0
  [[ "$(cache_get "$cache" CMAKE_C_COMPILER || true)" != "$cc" ]] && return 0
  [[ "$(cache_get "$cache" CMAKE_CXX_COMPILER || true)" != "$cxx" ]] && return 0
  # generator is stored in CMakeCache under CMAKE_GENERATOR (in CMakeFiles)
  if [[ -f "$bdir/CMakeFiles/CMakeOutput.log" || -f "$cache" ]]; then
    # Prefer reading from CMakeCache if present
    local genfile="$bdir/CMakeCache.txt"
    if grep -q '^CMAKE_GENERATOR:INTERNAL=' "$genfile" 2>/dev/null; then
      local g; g=$(awk -F= '/^CMAKE_GENERATOR:INTERNAL=/{print $2}' "$genfile")
      [[ "$g" != "$gen" ]] && return 0
    fi
  fi
  return 1
}

configure() {
  local bdir="$1" build_type="$2" build_editor="$3" cc="$4" cxx="$5" gen="$6"
  if need_configure "$bdir" "$build_type" "$build_editor" "$cc" "$cxx" "$gen"; then
    echo ""
    echo "Configuring CMake ($gen, type=$build_type, editor=$build_editor)..."
    cmake -S "$PROJECT_ROOT" -B "$bdir" \
      -DCMAKE_BUILD_TYPE="$build_type" \
      -DBUILD_EDITOR="$build_editor" \
      -DCMAKE_C_COMPILER="$cc" \
      -DCMAKE_CXX_COMPILER="$cxx" \
      -G "$gen"
  else
    echo ""
    echo "Skipping CMake configure. Cache matches."
  fi
}

build() {
  local bdir="$1" target="$2"
  echo ""
  echo "Building $target..."
  cmake --build "$bdir" --target "$target" -j "$(jobs)"
}

run_app() {
  local exe="$1" work="$2" name="$3"
  echo ""
  echo "=================================="
  echo "Build successful!"
  echo "=================================="
  echo ""
  read -r -p "Run the application? [Y/n]: " run_choice
  if [[ "$run_choice" =~ ^[Nn]$ ]]; then
    echo "Build complete. Executable at: $exe"
    exit 0
  fi
  echo ""
  echo "Running $name with working directory: $work"
  echo "=================================="
  echo ""
  (cd "$work" && "$exe")
}

# ---------- UI ----------
echo "=================================="
echo "   PiiXeL Engine Build & Run"
echo "=================================="
echo ""
echo "Select build target:"
echo "1) Editor (Debug)"
echo "2) Game (Release, standalone)"
echo "3) Editor (Release)"
echo "4) Build Tools (embed_asset_tool, build_package)"
echo "5) Build Game Package"
echo "6) Game (Debug, standalone with console)"
echo ""
read -r -p "Enter choice [1-6]: " choice

# ---------- defaults ----------
BUILD_DIR=""
BUILD_TYPE=""
BUILD_EDITOR=""
TARGET_NAME=""
EXECUTABLE_PATH=""
CC="${CC:-gcc}"
CXX="${CXX:-g++}"
GENERATOR="Unix Makefiles"

case "$choice" in
  1)
    echo "Building Editor (Debug)..."
    BUILD_DIR="$PROJECT_ROOT/build/editor"
    BUILD_TYPE="Debug"
    BUILD_EDITOR="ON"
    TARGET_NAME="editor"
    EXECUTABLE_PATH="$BUILD_DIR/games/MyFirstGame/editor"
    ;;
  2)
    echo "Building Game (Release, standalone)..."
    BUILD_DIR="$PROJECT_ROOT/build/game"
    BUILD_TYPE="Release"
    BUILD_EDITOR="OFF"
    TARGET_NAME="game"
    EXECUTABLE_PATH="$BUILD_DIR/games/MyFirstGame/game"
    ;;
  3)
    echo "Building Editor (Release)..."
    BUILD_DIR="$PROJECT_ROOT/build/editor-release"
    BUILD_TYPE="Release"
    BUILD_EDITOR="ON"
    TARGET_NAME="editor"
    EXECUTABLE_PATH="$BUILD_DIR/games/MyFirstGame/editor"
    ;;
  4)
    echo "Building Build Tools..."
    BUILD_DIR="$PROJECT_ROOT/build/tools"
    BUILD_TYPE="Release"
    BUILD_EDITOR="OFF"
    configure "$BUILD_DIR" "$BUILD_TYPE" "$BUILD_EDITOR" "$CC" "$CXX" "$GENERATOR"
    echo ""
    echo "Building embed_asset_tool..."
    build "$BUILD_DIR" "embed_asset_tool"
    echo ""
    echo "Building build_package..."
    build "$BUILD_DIR" "build_package"
    echo ""
    echo "=================================="
    echo "Tools built successfully!"
    echo "=================================="
    echo "  - embed_asset_tool: $BUILD_DIR/embed_asset_tool"
    echo "  - build_package: $BUILD_DIR/build_package"
    echo "=================================="
    exit 0
    ;;
  5)
    echo "Building Game Package..."
    BUILD_DIR="$PROJECT_ROOT/build/package"
    BUILD_TYPE="Release"
    BUILD_EDITOR="OFF"
    configure "$BUILD_DIR" "$BUILD_TYPE" "$BUILD_EDITOR" "$CC" "$CXX" "$GENERATOR"
    build "$BUILD_DIR" "build_package"
    echo ""
    echo "Running package builder..."
    (cd "$WORK_DIR" && "$BUILD_DIR/build_package")
    echo ""
    echo "=================================="
    echo "Package build complete!"
    echo "=================================="
    exit 0
    ;;
  6)
    echo "Building Game (Debug, standalone with console)..."
    BUILD_DIR="$PROJECT_ROOT/build/game-debug"
    BUILD_TYPE="Debug"
    BUILD_EDITOR="OFF"
    TARGET_NAME="game"
    EXECUTABLE_PATH="$BUILD_DIR/games/MyFirstGame/game"
    ;;
  *)
    echo "Invalid choice!"
    exit 1
    ;;
esac

configure "$BUILD_DIR" "$BUILD_TYPE" "$BUILD_EDITOR" "$CC" "$CXX" "$GENERATOR"
build "$BUILD_DIR" "$TARGET_NAME"

if [[ -f "$EXECUTABLE_PATH.exe" ]]; then
  EXECUTABLE_PATH="$EXECUTABLE_PATH.exe"
fi

if [[ ! -f "$EXECUTABLE_PATH" ]]; then
  echo ""
  echo "ERROR: Executable not found at: $EXECUTABLE_PATH"
  exit 1
fi

run_app "$EXECUTABLE_PATH" "$WORK_DIR" "$TARGET_NAME"
