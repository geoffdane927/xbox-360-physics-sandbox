# BlockWorks 360

Native Xbox 360 sandbox/editor built with the Xbox 360 XDK, Direct3D 9 and XInput.

## Controls and editor
- Start opens the pause menu. D-pad navigates; left/right changes the selected setting; A confirms; B resumes.
- Back switches Editor/Player mode.
- In Editor mode, left-stick click cycles the three inventory tabs. LB/RB selects an item; A places it; X removes the last block.
- The object/script rule system has been removed. No rule interpreter, rule arrays, script editor, or script menu remain.

## Physics
- Crates currently use simple gravity and block collision. Global gravity, box friction and air drag are configurable in Pause.
- The physics code is being expanded toward Verlet integration and connected constraints.
- Save data uses version 4 at Hdd1:\\BlockWorks360.sav. Older save versions are not compatible.

## Build
Open BlockWorks360.vcxproj in Visual Studio 2010 with the Xbox 360 XDK installed. Build Release | Xbox 360. Source changes have not been compiled or tested in the XDK or Xenia in this session.
