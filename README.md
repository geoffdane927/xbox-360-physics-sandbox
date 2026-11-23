# BlockWorks 360

Native Xbox 360 sandbox/editor using the Xbox 360 XDK, Direct3D 9 and XInput.

## Controls
- Start opens Pause. D-pad selects settings, left/right changes values, A confirms, B resumes.
- Back switches Editor/Player mode.
- In Editor mode, left-stick click switches hotbar pages; LB/RB selects items; A places; X removes the last block.
- The rule-based scripting system and object script editor have been removed.

## Current physics
Crates use basic gravity, block collision and player pushing. This is not yet full Verlet/constraint physics. The old save format is intentionally invalidated; the current version is 4 at `Hdd1:\\BlockWorks360.sav`.

## Build
Open BlockWorks360.vcxproj in Visual Studio 2010 with the Xbox 360 XDK installed. Build Release | Xbox 360. This revision has not been compiled or tested in Xenia in this session.
