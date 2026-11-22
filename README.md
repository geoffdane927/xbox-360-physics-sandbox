# BlockWorks 360

A modular Xbox 360 sandbox prototype using the Microsoft Xbox 360 XDK, Direct3D 9 and XInput. The project no longer uses XSDK or CMake.

## Project layout

- src/main.cpp — startup and main loop
- src/core/input.cpp — controller polling
- src/game/world.cpp — block storage and starter world
- src/game/physics.cpp — player/block collision
- src/game/player.cpp — movement, gravity, jumping and block placement
- src/render/renderer.cpp — Direct3D 9 rendering, filled triangles, HUD and split-screen
- src/game/game.h — shared structures and interfaces
- BlockWorks360.vcxproj — Visual Studio Xbox 360 project

## Build requirements

- Windows
- Full Microsoft Xbox 360 XDK installation, including its Visual Studio platform integration
- Visual Studio 2010 with the Xbox 360 platform installed

Open BlockWorks360.vcxproj in the XDK-enabled Visual Studio installation and select Release | Xbox 360. Merely having the XDK command-prompt utilities available is not sufficient if the Xbox 360 Visual Studio platform/toolset was not installed.

The project file is a starting point for the legacy XDK MSBuild platform. XDK versions can differ, so if Visual Studio reports a missing platform toolset or target file, the project settings must be matched to a sample project installed by that exact XDK version. The source has not been compiled against the user's local XDK installation.

## Controls

- Back toggles Editor / Player mode
- Start opens/closes the pause menu. In the menu, D-pad up/down selects, A confirms, left/right changes the manual render distance, and B resumes.
- Y exits
- Press Start on a second connected controller to join; each joined player gets a split-screen view.
- Left stick moves; right stick looks (up looks up, down looks down).
- Player mode: A jumps, X attacks/fires, LB/RB cycles item tabs. Players spawn unarmed and automatically collect sword/blaster pickups by walking over them. Sword and blaster have 100 durability; sword swings and blaster recoil animate, and use consumes durability. Green friendly NPCs and red sword / orange blaster enemies spawn in the world. Player 2+ presses Start to join.
- Editor mode: triggers move vertically, LB/RB cycles item tabs (Spike, Blaster pickup, Sword pickup, Checkpoint flag), A places the selected item, X removes the last placed object, B toggles grid. D-pad left/right selects block color.
- Pause menu: use Spawn X/Y/Z to adjust the shared respawn point, or select SET SPAWN HERE to copy your current position. A flag marks the spawn point. Spikes kill players on contact and respawn them at that point.

Up to four connected controllers are supported. Split-screen is disabled by default and only activates when the split_screen game setting is explicitly enabled. Render distance starts at 512 world units and scales with player count and measured frame rate; the grid extends up to 256 units from the player. New worlds start empty. The character is an upright box with basic gravity and AABB collision, not a ragdoll.

## Limitations

No level saving, node scripting, textured materials, lighting system, or ragdoll physics yet. The inventory is a simple color-selection HUD, not a full item system.
