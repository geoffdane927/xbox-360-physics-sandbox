# Xbox 360 Physics Sandbox

Native Xbox 360 XDK C++ project foundation.

## Current scope

This first milestone intentionally contains only:
- Xbox 360 controller polling through XInput;
- a minimal Direct3D 9 renderer;
- a simple 3D wireframe test scene;
- a free-fly camera controlled by the gamepad.

No editor, physics, Verlet solver, scripting, asset pipeline, or save system has been added yet.

## Build

Create an Xbox 360 Game/Application project using the template installed with your Xbox 360 XDK and add `src/main.cpp`. The source uses the XDK headers `xtl.h`, `d3d9.h`, and `xinput.h`.

Use the Xbox 360 target configuration supplied by your XDK. The XDK project template should provide the appropriate platform libraries and linker settings. XDK versions differ, so this repository does not guess at a Visual Studio project format.

## Controls

- Left stick: move camera horizontally
- Right stick: look around
- A: reset camera
- B: toggle the ground grid
- Start: reset camera
- Back: exit the title (debug/development use)

## Status

Source foundation only. It has not yet been compiled against a specific XDK installation.