# asteroidsSegaGenesis

MegaStroids version 2.0.0

Renamed application to MegaStroids as Asteroids is still a copyrighted title (I think). 2.0.0 has a couple of funny bugs I still need to patch out when I have time. In version 2.0.1 functionality should be baseline and then I can start adding planned features. Had to do a major rewrite because the original code wasn't dynamic enough for planned features. Here are a list of things that have been done and still need to be accomplished.

 Future Features:
 - Did a lot of work for prepping the game for cooperative play.
 - 4 player versus mode
 - Add explosion animations
 - Add enemy UFO to game (through ship struct pointed to appropriate sprite
 - Get GDB on blastem implemented correctly. (I'll include a guide to using GDB when I have time)
 - Add sound effects using the PSG channels. I may have some trouble with this as I'm still not very good with the Genesis sound chip. If it can't be done I'll implement it through the sample channel. Seems messy though....
 - Add options screen to the main menu
 - High score data saved to SRAM (Mainly to experiment with how data is pushed to such regions. SGDK handles the transaction but the adress needs to be set initially. Need to investigate this more.)

Change Log:

Version 2.0.1

 - Fixed bullet not firing bug
 - Fixed bullet acceleration bug
 - Added debug statements in fire bullets and update positions to look at bullet acceleration values
 - Fixed ship animation bug where the ship would not update rotated sprite
 - Removed bullet acceleration at FIX16(6) not necessary. Can reimplement limit in fire bullets function later if I need it.
 - 

Version 2.0.0
 - Added struct for differet entity type in game. Makes the code a lot easier to work on and add things easily as arrays can be expanded with all appropriate information for the entity.
 - Fixed structural problems program now executes in a more logical fashion. (For more details refer to update positions and levelSetup functions)
 - Deleted unnecessary code from the main loop.
 - optimized parameter passing to use 8-bit and 16-bit passed parameters. (There is a global variable that holds an 8-bit and 16-bit param). Compiler passes all parameters as 32-bit which can slow down the code.
 - animation frame counter is broken. Need to roll that into the ship struct.
 - collision us unimplemented for ship and rock collision (easy fix when I get around to it).
 

