# PiiXeL Engine

A modern 2D game engine with a complete editor, built using C++20.

## Features

- **Entity Component System (ECS)** powered by EnTT v3.15.0
- **2D Physics** using Box2D v3.1.1 (latest version)
- **Cross-platform rendering** with Raylib 5.5
- **Full-featured Editor** with ImGui v1.92.2
- **Custom Reflection System** for component serialization and inspector
- **Scene Management** with JSON serialization
- **Asset Packaging System** for distribution
- **Scripting System** with native C++ script components
- **Built-in Profiler** for performance analysis
- **Undo/Redo System** in the editor

## Core Technology Stack

- **C++20** - Modern C++ with explicit type declarations
- **EnTT v3.15.0** - Entity Component System for game architecture
- **Box2D v3.1.1** - Physics simulation (latest version 3.x)
- **Raylib 5.5** - Rendering, windowing, and input handling
- **ImGui v1.92.2** - Immediate mode GUI for the editor
- **rlImGui** - Integration layer between Raylib and ImGui
- **nlohmann/json v3.11.3** - JSON serialization

## Dependencies

All dependencies are automatically fetched and built via CMake FetchContent. No manual installation required.

### Required Dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| **Raylib** | 5.5 | Graphics, windowing, input, audio |
| **EnTT** | v3.15.0 | Entity Component System |
| **Box2D** | v3.1.1 | 2D Physics simulation |
| **nlohmann/json** | v3.11.3 | JSON serialization |
| **ImGui** | v1.92.2 (docking) | Editor UI (Editor mode only) |
| **rlImGui** | main | Raylib-ImGui integration (Editor mode only) |

### Build Requirements

- **CMake** 3.23 or higher
- **C++20 compatible compiler**:
  - **Windows**: MSVC 2019/2022 (Visual Studio 16.0+)
  - **Linux**: GCC 10+ or Clang 12+
  - **macOS**: AppleClang 13+ (Xcode 13+)

## Compiler Compatibility

PiiXeL Engine is designed to be fully compatible with both **GCC** and **MSVC** compilers:

### MSVC (Windows)
- Warning level: `/W4 /permissive-`
- Strict C++20 conformance
- Full support for Visual Studio 2019/2022

### GCC/Clang (Linux/macOS)
- Warning flags: `-Wall -Wextra -Wpedantic`
- Full C++20 support (concepts, ranges, etc.)
- Tested with GCC 10+ and Clang 12+

The codebase follows strict C++20 standards and compiles without warnings on both platforms.

## Quick Start

### Prerequisites

**Windows:**
- Visual Studio 2019/2022 with C++ Desktop Development workload
- CMake 3.23+
- Git

**Linux:**
- GCC 10+ or Clang 12+
- CMake 3.23+
- Git
- X11 development libraries: `sudo apt install libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev`

**macOS:**
- Xcode 13+ with Command Line Tools
- CMake 3.23+
- Git

## Building the Project

### Linux/macOS - Using build_and_run.sh

The easiest way to build on Linux/macOS is to use the provided build script:

```bash
chmod +x build_and_run.sh
./build_and_run.sh
```

This interactive script will prompt you to select a build target:

1. **Editor (Debug)** - Full editor with debugging symbols
2. **Game (Release, standalone)** - Standalone game without editor
3. **Editor (Release)** - Optimized editor build
4. **Build Tools** - Package builder tool
5. **Build Game Package** - Create distributable game package

The script automatically:
- Detects your compiler (GCC/Clang)
- Configures CMake with optimal settings
- Builds with all available CPU cores
- Runs the application after successful build

### Windows - Manual CMake Build

#### Editor Mode (with ImGui)

```bash
# Configure
cmake -B build/editor -DCMAKE_BUILD_TYPE=Debug -DBUILD_EDITOR=ON

# Build
cmake --build build/editor --config Debug

# Run
build/editor/games/MyFirstGame/Debug/editor.exe
```

#### Game Mode (standalone, no editor)

```bash
# Configure
cmake -B build/game -DCMAKE_BUILD_TYPE=Release -DBUILD_EDITOR=OFF

# Build
cmake --build build/game --config Release

# Run
build/game/games/MyFirstGame/Release/game.exe
```

### Using CMake Presets

The project includes CMake presets for common configurations:

```bash
# Configure with preset
cmake --preset editor          # Editor (Debug)
cmake --preset game            # Game (Release)
cmake --preset editor-release  # Editor (Release)

# Build with preset
cmake --build --preset editor
cmake --build --preset game
cmake --build --preset editor-release
```

### Using CLion (recommended)

1. Open the project folder in CLion
2. CLion will automatically detect CMake configuration
3. Select a preset from the toolbar: `editor`, `game`, or `editor-release`
4. Build and run using CLion's interface

## Build Modes

### Editor Mode (`BUILD_EDITOR=ON`)

- Includes full ImGui editor interface
- Scene editing with gizmos, hierarchy panel, and inspector
- Undo/Redo support
- Runtime scene manipulation
- Built-in profiler and console
- Suitable for development and content creation

### Game Mode (`BUILD_EDITOR=OFF`)

- No ImGui or editor dependencies
- Minimal executable for distribution
- Direct game rendering without editor overlay
- Optimized for performance
- Smaller binary size

## Project Structure

```
PiiXeLEngine/
├── engine/                    # Engine library
│   ├── src/                   # Engine source code
│   │   ├── Core/              # Core systems (Engine, Application)
│   │   ├── Systems/           # ECS Systems (Render, Physics, Script)
│   │   ├── Components/        # ECS Components
│   │   ├── Scene/             # Scene management
│   │   ├── Editor/            # Editor UI and commands
│   │   ├── Reflection/        # Reflection system
│   │   ├── Scripting/         # Script system
│   │   ├── Physics/           # Physics integration
│   │   ├── Resources/         # Asset management
│   │   └── Debug/             # Debug utilities
│   ├── include/               # Public headers
│   ├── assets/                # Engine assets (splash screen, etc.)
│   └── CMakeLists.txt
│
├── games/                     # Game projects
│   └── MyFirstGame/           # Example game
│       ├── src/               # Game-specific C++ code
│       │   ├── game_main.cpp  # Standalone game entry point
│       │   └── GameScripts.cpp # Custom game scripts
│       ├── include/           # Game headers
│       ├── assets/            # Game assets (textures, audio)
│       ├── scenes/            # Game scenes (.scene files)
│       ├── datas/             # Generated data (game.package)
│       ├── game.config.json   # Game configuration
│       └── CMakeLists.txt
│
├── tools/                     # Build tools
│   └── BuildPackage.cpp       # Game packaging tool
│
├── build/                     # Build output (generated)
│   ├── editor/                # Editor build
│   ├── game/                  # Game build
│   └── editor-release/        # Release editor build
│
├── docs/                      # Documentation
├── cmake/                     # CMake modules
├── CMakeLists.txt             # Root CMake configuration
├── CMakePresets.json          # CMake presets
├── build_and_run.sh           # Linux/macOS build script
└── README.md                  # This file
```

## Creating a New Game

### 1. Create Game Directory Structure

```bash
mkdir -p games/MyNewGame/{src,assets,scenes,datas,include}
```

### 2. Copy Base Files

Copy from `games/MyFirstGame/`:
- `CMakeLists.txt`
- `src/game_main.cpp`
- `src/GameScripts.cpp`
- `game.config.json`

### 3. Configure game.config.json

```json
{
  "title": "My New Game",
  "window": {
    "width": 1920,
    "height": 1080,
    "fullscreen": false,
    "vsync": true
  },
  "startScene": "scenes/MainScene.scene"
}
```

### 4. Build Your Game

**Linux/macOS:**
```bash
GAME_PROJECT=MyNewGame ./build_and_run.sh
```

**Windows:**
```bash
cmake -B build/editor -DGAME_PROJECT=MyNewGame -DBUILD_EDITOR=ON
cmake --build build/editor
```

### 5. Open in Editor

Run the editor executable and start creating your game!

## Game Packaging and Distribution

### Create a Game Package

The game package system bundles all assets and scenes into a single `.package` file:

**Linux/macOS:**
```bash
./build_and_run.sh
# Select option 5: Build Game Package
```

**Windows:**
```bash
cmake -B build/tools -DBUILD_EDITOR=OFF
cmake --build build/tools --target build_package
cd games/MyFirstGame
../../build/tools/build_package.exe
```

This generates `games/MyFirstGame/datas/game.package` containing:
- All scene files
- All assets (textures, audio, etc.)
- Game configuration

### Build Final Distributable

After creating the package, build the standalone game:

```bash
cmake -B build/game -DCMAKE_BUILD_TYPE=Release -DBUILD_EDITOR=OFF
cmake --build build/game --config Release
```

Distribute:
- The game executable (`game.exe` or `game`)
- The `datas/game.package` file
- Required DLLs (Windows only)

## Scripting System

PiiXeL Engine supports native C++ scripting with a component-based approach.

### Creating a Script Component

```cpp
// In games/MyFirstGame/src/GameScripts.cpp
#include "Scripting/ScriptComponent.hpp"

class PlayerController : public PiiXeL::ScriptComponent {
public:
    void OnStart() override {
        // Initialization
    }

    void OnUpdate(float deltaTime) override {
        // Per-frame logic
    }

    void OnCollision(entt::entity other) override {
        // Collision handling
    }
};

REGISTER_SCRIPT(PlayerController)
```

Scripts are attached to entities via the Script component in the editor.

## Editor Features

- **Scene Hierarchy** - Tree view of all entities
- **Inspector Panel** - Component property editing with reflection
- **Viewport** - Interactive scene view with gizmos
- **Console** - Runtime logging and messages
- **Profiler** - Performance monitoring
- **Undo/Redo** - Full command history (Ctrl+Z / Ctrl+Y)
- **Play/Pause/Step** - Runtime control
- **Scene Serialization** - Save/Load scenes (Ctrl+S / Ctrl+O)
- **Drag & Drop** - Asset management

## Coding Standards

- **Avoid `auto`** - Use explicit type declarations for clarity
- **Brace initialization** - Use `{}` for constructors (e.g., `Vector2{0.0f, 0.0f}`)
- **SOLID principles** - Clean, maintainable architecture
- **No RTTI** - Custom reflection system instead
- **Performance focus** - Cache-friendly data layouts with EnTT

## Development Workflow

### For Engine Development

1. Make changes to `engine/` code
2. Build with editor mode
3. Test in the editor
4. Ensure MSVC and GCC compatibility

### For Game Development

1. Open editor: `./build_and_run.sh` → Select "Editor (Debug)"
2. Create/edit scenes in the editor
3. Write custom scripts in `games/MyFirstGame/src/`
4. Test in editor
5. Create package for distribution
6. Build standalone game

## Troubleshooting

### CMake hangs during configuration (Windows)

If CMake gets stuck downloading dependencies:
1. Close CLion/Visual Studio completely
2. Delete the `build/` folder
3. Re-run CMake configuration

### Missing X11 libraries (Linux)

```bash
sudo apt install libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev
```

### Compiler not found (Linux)

Set explicit compiler:
```bash
export CC=gcc-11
export CXX=g++-11
./build_and_run.sh
```

### ImGui linker errors

Make sure you're building with `BUILD_EDITOR=ON` when using editor features.

## Contributing

Contributions are welcome! Please ensure:
- Code compiles without warnings on both MSVC and GCC
- Follow the coding standards (no comments, explicit types)
- Test on both Windows and Linux if possible
- Maintain C++20 standard compliance

## Acknowledgments

- **EnTT** - Michele Caini
- **Box2D** - Erin Catto
- **Raylib** - Ramon Santamaria
- **ImGui** - Omar Cornut
- **nlohmann/json** - Niels Lohmann

## Support

For issues, questions, or feature requests, please open an issue on the repository.

---

**Built with modern C++20 | Cross-platform | Open source**
