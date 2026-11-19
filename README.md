# Xbox 360 Physics Sandbox

Native Xbox 360 XDK C++ project foundation.

## Current scope

The current foundation includes:

- Xbox 360 XInput polling for up to four controllers;
- a Direct3D 9 wireframe renderer with a separate camera and viewport for each player;
- automatic local split-screen for connected controllers;
- a basic player/editor mode switch;
- an eight-slot color-coded inventory hotbar in player mode;
- simple block placement and editor removal controls;
- reduced view distance and a smaller grid when split-screen is active.

This is still an early graphics and input foundation. It does not yet include a physics simulation, polished 3D models, textures, text rendering, menus, level saving, or scripting.

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
- X: place the selected item as a block
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

The renderer lowers the view distance and grid detail as more players join. The player count is based on connected controllers.

## Status

Source foundation only. It has not yet been compiled against a specific XDK installation or tested on an Xbox 360. The renderer currently uses simple wireframe geometry and a minimal color hotbar; text, textures, and finished graphics are future work.
