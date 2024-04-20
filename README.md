# Sage Game Engine

A proof-of-concept 2D game engine. Created in University of Michigan's EECS 498-007 Game Engine Architecture course during Winter 2024.

Core ideas:
1. Actors are containers of components
2. Scenes are containers of actors
3. A game is a collection of scenes
4. Multiplayer games are made by replicating state

## Building From Source

Sage Engine uses CMake for its build system. To get started, clone the repository and initialize submodules:

```bash session
$ git clone https://github.com/dnsge/game-engine
$ cd game-engine
$ git submodules update --init --recursive
```

### macOS

```bash session
# Install dependencies via brew
$ brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer boost
# Initialize cmake build directory
$ mkdir build && cd build
$ cmake ..
# Build the engine
$ cmake --build .
# Engine will be built at bin/sge
```
### Windows

```bash session
$ vcpkg install
$ mkdir build && cd build
$ cmake "-DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" ..
$ cmake --build .
```
