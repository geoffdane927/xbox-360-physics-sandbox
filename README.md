# BlockWorks 360

Native Xbox 360 sandbox/editor using the Xbox 360 XDK, Direct3D 9 and XInput.

## Controls
- Start opens Pause. D-pad selects settings, left/right changes values, A confirms, B resumes.
- Back switches Editor/Player mode.
- In Editor mode, left-stick click (L3) cycles the three hotbar pages; LB/RB selects items; LT places at the face you're aiming at; D-pad Up/Down flies vertically; RT breaks the block you're aiming at (10-tile maximum range); X removes the last block.
- Hold Y to use the physics gun: aim at a crate to grab it, move/look to reposition it, and release Y to drop it. RT is reserved for block breaking in the editor.
- The rule-based scripting system and object script editor have been removed.

## Current physics
Crates use Verlet position integration, per-box mass/friction/gravity, block and box collision, and iterative constraints. Editor page 3 contains glue, rope, hinge and motor links, plus ice and crate placement. Aim at one crate and press A, then aim at a second crate and press A to connect them. LT places selected blocks and objects. Glue locks distance, rope prevents stretching, hinge is a softer distance joint, and motor varies its target length. Save format is version 5 at `Hdd1:\\BlockWorks360.sav`.

## Build
Open BlockWorks360.vcxproj in Visual Studio 2010 with the Xbox 360 XDK installed. Build Release | Xbox 360. This revision has not been compiled or tested with the XDK or Xenia in this session.
