> [!IMPORTANT]  
> Daxanadu is not a clone, a remake nor a source port. Daxanadu is an NES emulator that only works with a Faxanadu rom file. You must own a legal copy of that rom. I will not provide it for you, so don't ask.

![](images/Daxanadu.png)

# DAXANADU

Daxanadu stands for "Daivuk's Xanadu". It is a patched version of Faxanadu for the NES. It is not a clone or a remake, but rather it is an NES emulator tailored for Faxanadu. To run the game, you will need to put the Faxanadu rom file into the same folder as Daxanadu. The rom must be the English version. Rename it to exactly: `Faxanadu (U).nes`. The other US version with the fixed font might work, but I have not tested it as I do not own a copy of this version.

Daxanadu dynamically patches the rom as the game run to allow for easy option menus that toggle the options in runtime.

## New features
Here is the list of improvements, options and features that Daxanadu brings to Faxanadu.
- Meditating saves state instead of showing a password.
- "Continue" game loads save state instead of asking for password.
- Rebindable inputs.
- Separated adjustable volume for music and sfx.
- Various gameplay options, [see the Options category for more detail](##Options).

## Options
### Gameplay options
- `DIALOG SPEED`: Increases NPCs dialog speed.
- `KEEP GOLD ON DEATH`: Don't reset gold to your level when dying.
- `KEEP XP ON DEATH`: Don't reset xp to your level when dying.
- `DOUBLE GOLD`: Double gold reward from enemies.
- `DOUBLE XP`: Double xp reward from enemies.
- `KING GOLDS`: Fix the issue where the king can give you 1500 "golds" more than once.
- `COINS DESPAWN`: Allow coins to stay indefinitely on screen like bread.
- `EQUIP IN SHOPS`: Allow to equip inventory in shops.
- `ITEM ROOM COUNTER`: Reduce item room counter to 1. Certain "secret" items only appear after killing all enemies in the room, if the counter is exactly 4. Forcing the player to attempt up to 4 times.
- `XP AFFECTS WINGBOOTS`: Wingboots always last 40 seconds.
- `XP AFFECTS SPEED`: Character speed is always at max.
- `FIX PENDANT`: Reverse the pendant effect. Starting weak and getting strong once finding the pendant.

### Visual options
- `MIST SCROLL`: All fog sprites are animated at different speed for a slight parallax effect.
- `CIGARETTES`: Remove cigarettes and smoke from NPCs.
- `LEVEL NAME POPUP`: Show a popup at the top of the screen when entering an area, telling its name.

### New game options
- `START FULL HEALTH`: Starts the game with full health and full mana.

## Save states
You can at all time save a state using LEFT CTRL + number from 1 to 9. To load a state, press a number from 1 to 9.

## Controllers
Note that for now, only the XBOX controller through XInput is supported. Mouse and keyboard are also supported.
I have plans to support more controllers through SDL2 in the future.
