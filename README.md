# BlockWorks 360

Native Xbox 360 sandbox/editor using the Xbox 360 XDK, Direct3D 9 and XInput.

## Editor and object scripts

- Left-stick click switches between Blocks and Objects hotbar pages.
- Aim at an NPC, crate, block, spike, or pickup and press Y to open that object's script editor.
- The script editor starts with the selected object as the rule owner. Press B or Y to close it.
- Use D-pad up/down to move through the menu, left/right to change values, and A to add a rule.
- You can also open Pause with Start and choose PLAYER as the script owner to create player rules.

## Script conditions

SEE PLAYER, BUMP GREEN, GET SHOT, TOUCH PLAYER, TIMER, NEAR ITEM, ON DEATH, ON FALL, ON STOP FALLING, ON LAND, ON JUMP, ON USE.

## Script actions

MOVE TOWARD, MOVE BACK, TURN AROUND, CHANGE GREEN, DISAPPEAR, RESPAWN, JUMP, SHOOT, MOVE CAMERA, SUMMON, PLACE BLOCK, CUSTOM WEAPON.

MOVE TOWARD has a target type and target index selector (player, NPC, crate, block, pickup). SUMMON selects a template (friendly NPC, sword NPC, blaster NPC, crate). Summoned NPCs/crates inherit the source object's other script rules, so you can make scripted reusable actors.

Player scripts support camera-direction movement, jumping, respawning, placing blocks, summoning scripted actors, and custom projectile weapons. Example: set owner to PLAYER, IF to ON USE, DO to CUSTOM WEAPON.

Examples:
- IF SEE PLAYER -> MOVE TOWARD (target PLAYER)
- IF BUMP GREEN -> MOVE BACK
- IF GET SHOT -> DISAPPEAR
- IF ON FALL -> MOVE CAMERA
- IF ON USE -> CUSTOM WEAPON
- IF TIMER -> SUMMON (choose a template)

## Physics and play

Crates fall under gravity, land on the floor and blocks, and can be pushed. NPCs follow/chase or fire. Players can jump, collide, collect weapons, and use weapon durability. Spikes respawn players at the checkpoint.

## Saving

Pause includes SAVE GAME and LOAD GAME. The development save writes Hdd1:\\BlockWorks360.sav. The format version has been advanced for the expanded rule structure. This is intended for persistent devkit/Xenia HDD storage, not retail profile-based storage packages.

## Build

Open BlockWorks360.vcxproj in Visual Studio 2010 with the Xbox 360 XDK installed. Build Release | Xbox 360. Source changes have not been compiled or tested in the XDK or Xenia in this session.
