# VOID — Horror FPS

![VOID Logo](https://raw.githubusercontent.com/onurdrsn/void-game/main/assets/logo.png) <!-- Placeholder for potential logo -->

**VOID** is a minimalist, procedurally generated horror FPS built for the demoscene spirit. It features a dark, atmospheric underground research facility, atmospheric lighting, and high-performance rendering, all packed into a tiny executable.

---

## 👁️ Overview

Experience terror in a ~64 KB package. VOID is designed with extreme technical constraints, utilizing procedural generation for all assets—including textures, geometry, and sound—ensuring a unique experience without the bulk of traditional game assets.

### Key Features
- **Procedural World:** Every corridor and room is generated at runtime.
- **Dynamic Lighting:** Real-time flashlight and point light systems.
- **Atmospheric Audio:** Built-in 3D audio via `miniaudio`.
- **Extreme Optimization:** Target execution size of ~64 KB (UPX compressed).
- **Zero External Dependencies:** Built from scratch using only system APIs and a single header for audio.

---

## 🛠️ Technical Constraints

VOID is a testament to systems programming and minimalist design:
- **Language:** C++17 (No STL, No Exceptions, No RTTI).
- **Graphics:** OpenGL 3.3 Core Profile.
- **Memory:** Custom Arena Allocator (Zero dynamic allocation after initialization).
- **Proceduralism:** Shaders, textures, and geometry are all generated code.
- **Size:** Optimized for sub-100KB binaries using `UPX` compression.

---

## ⚡ Quick Start (Linux / WSL2 / Windows)

### 1. Prerequisites

Choose your distribution and run the appropriate command below:

#### **Arch Linux / Manjaro**
```bash
sudo pacman -S --needed base-devel cmake gcc mesa libx11 libxrandr libxext libxfixes libxi upx
```

#### **Ubuntu / Debian / Linux Mint**
```bash
sudo apt update && sudo apt install -y build-essential cmake gcc mesa-common-dev libx11-dev libxrandr-dev libxext-dev libxfixes-dev libxi-dev upx
```

#### **openSUSE**
```bash
sudo zypper install cmake gcc gcc-c++ mesa-libGL-devel libX11-devel libXrandr-devel libXext-devel libXfixes-devel libXi-devel upx
```

#### **Red Hat / Fedora / CentOS**
```bash
sudo dnf install cmake gcc gcc-c++ mesa-libGL-devel libX11-devel libXrandr-devel libXext-devel libXfixes-devel libXi-devel upx
```

#### **Windows (WSL2)**
First, ensure you have WSL2 installed. Then run the Ubuntu commands above inside your WSL2 terminal.

#### **Windows (Native - Optional)**
For native Windows builds, install:
- [CMake](https://cmake.org/download/)
- [Visual Studio Community](https://visualstudio.microsoft.com/downloads/) (with C++ workload)
- [OpenGL libraries](https://www.khronos.org/opengl/wiki/Getting_started#Downloading_and_installing_the_library)

### 2. Build Instructions
Use the provided `build.sh` script to compile the project:
```bash
chmod +x build.sh
./build.sh
```
The executable will be located in the `build/` directory.

### 3. Running the Game
```bash
cd build
./void_game
```
*Note: If you are on WSL2 without a GPU pass-through, you may need to use `LIBGL_ALWAYS_SOFTWARE=1 ./void_game`.*

---

## 🎮 Controls
- **WASD:** Movement
- **Mouse:** Look around
- **Space:** Jump
- **Shift:** Sprint
- **F:** Toggle Flashlight (Coming Soon)
- **Esc:** Exit

---

## 📂 Project Structure
```text
void-game/
├── src/
│   ├── audio.h/cpp      # Miniaudio Integration
│   ├── entity.h/cpp     #
│   ├── game.h/cpp       # Game Logic & State
│   ├── main.cpp         # Entry point & Main loop
│   ├── miniaudio.h      #
│   ├── platform.h/cpp   # Cross-platform Windowing & Input (X11/Win32)
│   ├── renderer.h/cpp   # OpenGL 3.3 Dynamic Renderer
│   ├── vmath.h/cpp      # Custom Math Library
├── CMakeLists.txt       # Build Configuration
└── build.sh             # Linux Build Script
```

---

## 📜 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
