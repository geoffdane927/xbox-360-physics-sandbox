# BlockWorks 360

Native Xbox 360 sandbox/editor using the Xbox 360 XDK, Direct3D 9 and XInput.

## Editor

- Left-stick click switches between two hotbar pages.
- Page 1 — Blocks: eight block colors. D-pad left/right or LB/RB selects a color.
- Page 2 — Objects: spike, blaster pickup, sword pickup, checkpoint, friendly NPC, sword enemy, blaster enemy, and pushable crate.
- A places the selected item; X removes the most recently added block (or deactivates the most recent crate); B toggles the grid.
- Back switches Editor/Player. Triggers move vertically in the editor.

## Physics and play

- Players jump, collide with blocks, collect weapons, and use weapon durability.
- Crates fall under gravity, land on the floor/blocks, and can be pushed by walking into them.
- Spikes respawn players at the checkpoint.
- Friendly NPCs follow; enemy NPCs chase or fire projectiles.
- Additional controllers press Start to join and receive split-screen views.

## Kodu-style rules

Open the pause menu with Start. Use D-pad up/down to choose a row and left/right to change values. The script rows let you select an NPC, choose an IF condition, choose a DO action, then select ADD RULE.

Conditions: SEE PLAYER, BUMP GREEN WALL, GET SHOT, TOUCH PLAYER, TIMER, NEAR ITEM.

Actions: MOVE TOWARD, MOVE BACKWARD, TURN, CHANGE GREEN, DISAPPEAR, RESPAWN, JUMP, SHOOT.

Examples:
- IF SEE PLAYER -> MOVE TOWARD
- IF BUMP GREEN WALL -> MOVE BACKWARD
- IF GET SHOT -> DISAPPEAR

Rules are evaluated while the world runs. Multiple rules can be assigned to the same NPC.

## Saving

The pause menu includes SAVE GAME and LOAD GAME. This development build writes to Hdd1:\\BlockWorks360.sav using the XDK CRT file API. It targets the persistent HDD volume used by a devkit/Xenia setup; it is not integrated with the retail Xbox 360 profile/device-selector save-game package system. Retail-ready saves require the XContent storage UI and content-package lifecycle.

## Build

Open BlockWorks360.vcxproj in Visual Studio 2010 with the Xbox 360 XDK platform installed. Build Release | Xbox 360.

The source has not been compiled or tested with the user's XDK/Xenia in this session.
