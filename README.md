# Xbox 360 Physics Sandbox

Native Xbox 360 XDK C++ test project.

## Current test

This build is a small playable graphics and collision test, not the finished sandbox engine.

- Filled triangle rendering for solid colored block geometry, with dark edge lines.
- A simple visible player character (blocky body and head).
- Basic upright character movement, gravity, jumping, floor collision, and AABB collision against placed blocks.
- Placeable blocks with eight selectable inventory colors.
- Editor and Player modes, switched globally with **Back**.
- Up to four local controllers with split-screen viewports.
- Frame-rate-aware graphics quality: render distance and grid density reduce when performance drops, then recover gradually.
- 60 FPS target/cap using a 60 Hz presentation interval plus a frame pacing wait.

The character is intentionally not a ragdoll. It stays upright and uses basic movement/gravity. There is no knockdown system or ragdoll solver yet.

## Build

Create an Xbox 360 Game/Application project using the template installed with your Xbox 360 XDK and add `src/main.cpp`.

Use the Xbox 360 target configuration supplied by your XDK. The XDK project template should provide the appropriate platform libraries and linker settings. XDK versions differ, so this repository does not guess at a Visual Studio project format.

## Controls

Press **Back** on any connected controller to switch the whole game between Editor and Player mode.

### Player mode

- Left stick: move
- Right stick: look / camera
- A: jump
- D-pad left/right: select inventory slot
- X: place the selected block
- Start: recenter your camera

### Editor mode

- Left stick: move horizontally
- Right stick: look / camera
- Left trigger / right trigger: move down / up
- A: place the selected block
- X: remove the most recently placed block
- B: toggle the ground grid
- Start: recenter your camera

## Split-screen

Each connected controller gets its own view, up to four players:

- 1 player: full screen
- 2 players: two side-by-side views
- 3–4 players: a 2-by-2 layout

The base render distance and grid detail are lower with more players. If the frame rate falls below the target, the renderer reduces render distance and grid density; it restores quality gradually when performance improves.

## Graphics and physics status

The renderer now submits filled triangle lists for solid block and character geometry, plus line lists for the grid and outlines. Collision is currently a simple upright player AABB against axis-aligned block AABBs, with per-axis horizontal movement resolution and swept vertical landing checks. This is suitable for a basic test scene, not a full physics engine.

## Testing status

The source has not been compiled against a specific Xbox 360 XDK installation or tested on an Xbox 360. Performance, exact frame pacing, and XDK API compatibility still need on-console verification.
