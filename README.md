# BlockWorks 360

A modular Xbox 360 homebrew sandbox test built with **XSDK** (the independent xenosworks toolchain), not the Microsoft XDK.

## Project layout

- `src/main.c` — startup, main loop, mode switching and app lifecycle.
- `src/core/input.c` — XSDK controller polling and edge-triggered button input.
- `src/game/world.c` — world/block storage and initial test scene.
- `src/game/physics.c` — upright player-vs-block AABB collision.
- `src/game/player.c` — movement, gravity, jumping, placement and editor controls.
- `src/render/renderer.c` — XSDK GPU setup, camera projection, triangle generation, split-screen rendering.
- `src/render/world.vert`, `world.frag` — GLSL shaders compiled by XSDK's shader pipeline.
- `src/game/game.h` — shared game data and module interfaces.

Each feature is kept in its own source file so changes to physics, input, world data or rendering do not require rewriting the application entry point.

## Build requirements

Follow the official [XSDK getting started guide](https://github.com/xenosworks/xsdk/blob/main/docs/GETTING_STARTED.md). XSDK builds on Linux or Windows through WSL 2. It needs its `powerpc-xsdk-elf` GCC toolchain, CMake 3.25+, Ninja and the XSDK repository/library.

This title requires an **RGH or JTAG Xbox 360** capable of launching unsigned homebrew XEX files. It will not run on a stock retail console.

From this repository, with XSDK installed at `$HOME/xsdk` and its compiler at `$HOME/xsdk-gcc`:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/xsdk/cmake/xsdk.cmake \
  -DXSDK_TOOLCHAIN=$HOME/xsdk-gcc \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Output: `build/blockworks360.xex`.

## Current controls

- **Back:** toggle Editor/Player mode.
- **Guide:** exit the title.
- **Player:** left stick moves, right stick looks, A jumps, D-pad left/right selects the block colour, X places a block, Start resets the player.
- **Editor:** left stick moves, right stick looks, triggers move vertically, A places a block, X removes the most recently placed block, B toggles the grid flag, Start resets the player.
- Up to four connected controllers get split-screen viewports.

## Current implementation notes

The player is an upright box with simple gravity, jumping and AABB collision. It is not a ragdoll and there is no knockdown system. The world stores up to 256 axis-aligned blocks. The renderer uses XSDK's Xenos GPU API and GLSL shaders; it does not use Direct3D 9 or Microsoft XDK headers.

This is an initial XSDK port and modularization. It has not yet been built with the user's local XSDK toolchain or tested on a console. The renderer uses a fixed 60 Hz simulation step paced by presentation/vblank. Render distance is reduced for split-screen; automatic frame-time quality scaling is still to be reimplemented.
