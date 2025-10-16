# PiiXeL Engine

A modern 2D game engine with a complete editor, built using C++20.

## Features

* **Entity Component System (ECS)** powered by EnTT v3.15.0
* **2D Physics** using Box2D v3.1.1
* **Cross-platform rendering** with Raylib 5.5
* **Full-featured Editor** with ImGui v1.92.2
* **Scripting System** with native C++ script components
* **Scene Management** with JSON serialization
* **Asset System** with UUID-based references and .pxa format
* **Build & Export** system with visual progress tracking
* **Built-in Profiler** for performance analysis
* **Undo/Redo System** in the editor

---

## Quick Start

### Prerequisites

**Windows:**

* Visual Studio 2019/2022 with C++ Desktop Development workload
* CMake 3.23+

**Linux:**

* GCC 10+ or Clang 12+
* CMake 3.23+
* X11 libraries:

  ```bash
  sudo apt install libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev
  ```

**macOS:**

* Xcode 13+ with Command Line Tools
* CMake 3.23+

All dependencies (Raylib, EnTT, Box2D, ImGui, nlohmann/json) are automatically fetched via CMake.

---

### Building

**Linux/macOS:**

```bash
chmod +x build_and_run.sh
./build_and_run.sh
# Select: 1. Editor (Debug)
```

**Windows (CLion):**

1. Open project in CLion
2. Select `editor` preset from toolbar
3. **Set Working Directory:**

    * Go to **Run → Edit Configurations…**
    * In **Working Directory**, set:

      ```
      $ContentRoot$\games\MyFirstGame
      ```
    * Replace `<project_root>` with the absolute path to your local PiiXeLEngine folder.
    * You must configure this for each project (`MyFirstGame`, `MyNewGame`, etc.).
4. Build and run

**Windows (Command Line):**

```bash
cmake -B build/editor -DCMAKE_BUILD_TYPE=Debug -DBUILD_EDITOR=ON
cmake --build build/editor --config Debug
cd games/MyFirstGame
../../build/editor/games/MyFirstGame/Debug/editor.exe
```

---

### CMake Presets

```bash
cmake --preset editor          # Editor with debugging
cmake --preset editor-release  # Optimized editor
cmake --preset game            # Standalone game (no console)
cmake --preset game-debug      # Standalone with console & debug symbols

cmake --build --preset editor
```

---

## Working Directory Configuration (Important)

To run the **Editor** or a **Game**, it is strongly recommended to use the following working directory:

```
<project_root>/games/MyFirstGame
```

This ensures that all relative asset paths, scene files, and packages are correctly resolved at runtime.
If not set, the engine may fail to locate assets (`.pxa` files, textures, etc.).

### How to configure in CLion

1. Go to **Run → Edit Configurations…**
2. Select your run target (e.g., `editor`)
3. In **Working Directory**, enter:

   ```
   $PROJECT_DIR$/games/MyFirstGame
   ```
4. Apply and repeat this for each game project (e.g., `MyNewGame`, etc.)

---

## Build Modes

* **Editor Mode** (`BUILD_EDITOR=ON`) - Full editor with scene editing, profiler, console
* **Game Mode** (`BUILD_EDITOR=OFF`) - Standalone executable without editor

**Game variants:**

* `game` preset: Release, no console (for distribution)
* `game-debug` preset: Debug symbols + console (for debugging)

---

## Project Structure

```
PiiXeLEngine/
├── engine/           # Engine library
│   ├── src/          # Engine source code
│   ├── include/      # Public headers
│   └── assets/       # Engine assets (splash screen, etc.)
├── games/            # Game projects
│   └── MyFirstGame/
│       ├── src/      # Game-specific C++ code
│       ├── include/  # Game scripts
│       ├── content/  # Game assets (textures, audio, scenes)
│       └── datas/    # Generated data (game.package)
├── tools/            # Build tools (package builder, asset embedder)
├── docs/             # Documentation
└── CMakeLists.txt
```

---

## Documentation

See **[docs/](docs/)** for detailed guides:

* **[How to Add a Component](docs/How-to-add-a-component.md)**
* **[How to Create a Script](docs/How-to-create-a-script.md)**
* **[How to Use Animations](docs/How-to-use-animations.md)**
* **[How to Use Physics](docs/How-to-use-physics.md)**
* **[How to Manage Assets](docs/How-to-manage-assets.md)**

---

## Asset System

PiiXeL Engine uses **UUID-based asset references** with a `.pxa` binary format:

* Place source assets (PNG, WAV, etc.) in your project
* Editor automatically generates `.pxa` files on import
* Assets tracked by UUID for cross-scene references
* Fast loading with embedded, optimized data

**Development:** Assets loaded directly from `.pxa` files
**Distribution:** Assets bundled in `game.package` file

**Git Strategy:** Commit `.pxa` files alongside sources. Use Git LFS for large projects (>100MB).

---

## Creating a New Game

```bash
# 1. Create directory structure
mkdir -p games/MyNewGame/{src,content,datas,include}

# 2. Copy base files from MyFirstGame
cp games/MyFirstGame/CMakeLists.txt games/MyNewGame/
cp games/MyFirstGame/src/game_main.cpp games/MyNewGame/src/
cp games/MyFirstGame/game.config.json games/MyNewGame/

# 3. Build
cmake -B build/editor -DGAME_PROJECT=MyNewGame -DBUILD_EDITOR=ON
cmake --build build/editor
```

**Then configure the working directory:**

```
$PROJECT_DIR$/games/MyNewGame
```

---

## Scripting Example

```cpp
#include "Scripting/ScriptComponent.hpp"

class PlayerController : public PiiXeL::ScriptComponent {
public:
    float moveSpeed{300.0f};
    float jumpForce{500.0f};

private:
    std::optional<PiiXeL::RigidBodyHandle> m_RigidBody;

    void OnStart() override {
        m_RigidBody = GetHandle<PiiXeL::RigidBody2D>();
    }

    void OnUpdate(float deltaTime) override {
        if (!m_RigidBody) return;

        Vector2 velocity = m_RigidBody->GetVelocity();
        velocity.x = 0.0f;

        if (IsKeyDown(KEY_RIGHT)) velocity.x = moveSpeed;
        if (IsKeyDown(KEY_LEFT)) velocity.x = -moveSpeed;

        if (IsKeyPressed(KEY_SPACE) && m_RigidBody->IsGrounded()) {
            m_RigidBody->AddImpulse(Vector2{0.0f, -jumpForce});
        }

        m_RigidBody->SetVelocity(velocity);
    }
};

REGISTER_SCRIPT(PlayerController)
```

---

## Build & Export

### From Editor (Recommended)

Use the **Build & Export** panel:

* **Build Game Package** – Creates `game.package` with all assets
* **Build Game Executable** – Compiles standalone game (Release)
* **Export Game** – Complete export ready to distribute

### From Command Line

```bash
./build_and_run.sh  # Select option 5
cmake --preset game
cmake --build --preset game
```

**Distribute:**

* Game executable (`game.exe` or `game`)
* `datas/game.package`

---

## Editor Features

* Scene Hierarchy
* Inspector with automatic UI generation
* Viewport with gizmos
* Console with runtime logging
* Profiler
* Asset Browser
* Undo/Redo
* Play/Pause/Step controls
* Scene save/load
* Build & Export with progress tracking

---

## Coding Standards

* Avoid `auto`
* Use `{}` for constructors
* Follow SOLID principles
* Use C++20 features only

---

## Troubleshooting

**CMake hangs (Windows):**

```bash
# Close IDE, delete build/ folder, restart
```

**Missing X11 libraries (Linux):**

```bash
sudo apt install libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev
```

**Compiler not found (Linux):**

```bash
export CC=gcc-11
export CXX=g++-11
./build_and_run.sh
```

---

## License

This project is licensed under the **MIT License**.  
See the [LICENSE](LICENSE) file for full details.

---

## Acknowledgments

* **EnTT** – Michele Caini
* **Box2D** – Erin Catto
* **Raylib** – Ramon Santamaria
* **ImGui** – Omar Cornut
* **nlohmann/json** – Niels Lohmann

---

**Built with modern C++20 | Cross-platform | Open source**
