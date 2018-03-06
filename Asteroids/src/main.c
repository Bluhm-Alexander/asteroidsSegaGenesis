#include <genesis.h>
#include <timer.h>
#include "sprite.h"

Sprite* sprites[64];

//All Sprites Coordinates and Position and hit boxes
fix16 positions[64][2];
fix16 velocity[64][2];
u8 indexBullets;
u8 hitBox[64];
//Controller status values
s8 pressed;
u8 counter;

//Ship Rotation in degrees
fix16 rotation;

//Prototypes in order of which they are called.
static void addCounter();
static void handleInput();
u16 convertToTable(fix16 degrees);
static void accelarateShip();
static void shipAnimation();
static void updatePositions();
static void fireBullets();
u8 checkCollision(fix16 aX, fix16 aY, fix16 bX, fix16 bY);


/**************************************************************
*              !-- Create Asteroid --!
* takes the number of the asteroid hit deletes it. I'm filling
* this in later.
**************************************************************/

void createAsteroid(u8 hitAsteroid)
{

}

/***************************************************************
* This is the Main Function I'm initializing the Video Chip
* I'm also going to keep all of variables in here maybe
* Main Game Loop is in main.
***************************************************************/

int main() {

    //VDP_drawText("Hello World!", 10, 13);

    //These are variables its always best to use everything sparingly because there is not much
    //space in memory
    u16 palette[64];
    u16 ind;
    char str[16];

    // disable interrupt when accessing VDP
    SYS_disableInts();
    // initialization
    VDP_setScreenWidth320();

     // init sprites engine
    SPR_init(120, 256, 256);

    // set all palette to black
    VDP_setPaletteColors(0, (u16*) palette_black, 64);

    // load background
    ind = TILE_USERINDEX;

    // VDP process done, we can re enable interrupts
    // We are re-enabling interrupts you can now draw sprites FOREVER until
    // reloading the background planes.
    SYS_enableInts();

    //Set default coordinates and set up game
    //default rotation
    rotation = 0;  //These variables should be in a game set up function
    indexBullets = 32;
    counter = 0;
    pressed = -1;

    u8 index;
    for (index = 0; index < 64; index++)
    {
        positions[index][0] = FIX16(400);
        hitBox[index] = 0;
    }

    positions[0][0] = FIX16(48);
    positions[0][1] = FIX16(192);
    positions[1][0] = FIX16(48);
    positions[1][1] = FIX16(0);
    positions[2][0] = FIX16(36);
    positions[2][1] = FIX16(36);

    //Set velocity for asteroids
    velocity[0][0] = FIX16(0.7);
    velocity[0][1] = FIX16(-0.7);
    velocity[1][0] = FIX16(0.5);
    velocity[1][1] = FIX16(0.7);
    velocity[2][0] = FIX16(0);
    velocity[2][1] = FIX16(0);

    //Draw the sprites on the screen and then update the VDP
    // First 5 is the X coordinates second 5 is y coordinates
    sprites[0] = SPR_addSprite(&asteroid_sprite, fix16ToInt(positions[0][0]), fix16ToInt(positions[0][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    sprites[1] = SPR_addSprite(&asteroid_sprite, fix16ToInt(positions[1][0]), fix16ToInt(positions[1][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    sprites[2] = SPR_addSprite(&medium_asteroid, fix16ToInt(positions[2][0]), fix16ToInt(positions[2][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));

    //Initialize the Ship
    //Setting ship position
    positions[62][0] = FIX16(160);
    positions[62][1] = FIX16(112);
    sprites[62] = SPR_addSprite(&ship_sprite, fix16ToInt(positions[62][0]), fix16ToInt(positions[62][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    //SPR_setAnim(sprites[62], 0);

    //Default ship resting
    shipAnimation();

    //Updating screens
    SPR_update();

    //Copy the colors into memory after updating the sprite engine
    memcpy(&palette[0], asteroid_sprite.palette->data, 16 * 2);

    //Fade in for kicks and giggles Fade in is 60 milliseconds it looks like or cycles
    VDP_fadeIn(0, (4 * 16) - 1, palette, 60, FALSE);
    //Apparently pointers are needed
    fix32ToStr(getFPS(), str, 2);
    while(1)
	{
		//read input
		handleInput();

		//move sprite
		updatePositions();

		//checkCollision();
		//update score
		//draw current screen (logo, start screen, settings, game, gameover, credits...)

    //Update sprites again
		SPR_update();

		//wait for screen refresh
		VDP_waitVSync();

		fix32ToStr(getTimeAsFix32(getTick()), str, 2);
		VDP_drawText(str, 10, 13);

		//I'm putting in a counter to time my rapid fire bullets
		//There has to be a better way to do this

        addCounter();
	}
	return (0);
}

/************************************************************************
*                       !---counter---!
*
* This is for the handle input function and the ships rapid fire
* function increments counter and raps it every 60 ticks.
************************************************************************/

static void addCounter()
{
    counter += 1;
    if (counter > 2)
    {
        counter = 0;
    }
}

/************************************************************************
*                       !----Handle Input----!
* Return type void!
* No Parameters
*
* Variables:
* unsigned 16 bit integer - frame
* unsigned 16 bit integer - value: holds JoyPad Inputs
*
* value is compared with BUTTON_UP which is a compiler defined variable
* for when the DPAD UP button is pressed down.
*
* This function Modifies the rotation variable by a factor of 9
* Function also modifies ship velocity at [62].
*
* The only inputs I need to worry about here are the DPAD and the A, B
* and C buttons all of which will fire a bullet. Right now the animation
* method is located inside handle Inputs I must remove this.
*
* I want handle inputs to call the ship animations function or
* acceleration functions only when the buttons are pressed thus making
* things a little more efficient.
************************************************************************/

static void handleInput()
{
    //Get the value from the controller
    u16 value = JOY_readJoypad(JOY_1);

    //Up button does nothing for now will control thrust
    if (value & BUTTON_UP)
    {
        accelarateShip();
    }

    if (value & BUTTON_DOWN)
    {

    }

    //Increment angle by 3 and then integer math
    if (value & BUTTON_LEFT)
    {
        rotation = fix16Add(rotation, FIX16(9));

        //Make sure the rotation is not less than 0
        if (rotation >= FIX16(360))
        {
            rotation = FIX16(0);
        }
        //Now update the ship animation.
        shipAnimation();
    }

    else if (value & BUTTON_RIGHT)
    {
        rotation = fix16Add(rotation, FIX16(-9));
        //Make sure degrees does not exceed 360
        if (rotation < FIX16(0))
        {
            rotation = FIX16(359);
        }
        //Now we update the animation
        shipAnimation();
    }


    if (value & BUTTON_A)
    {
        if (pressed == -1)
        {
            fireBullets();
            pressed = counter;
        }

        else if (counter == pressed /*&& pressed > 0*/)
        {
            fireBullets();
        }
        //fireBullets();
    }

    else
    {
        pressed = -1;
    }

    //Display the pressed variable
    /*char rad[16];
    fix16ToStr(pressed, rad, 2);
    VDP_drawText(rad, 14, 16);*/
    //I think there may be problems putting these in controller inputs
    /*if (rotation < FIX16(0))
        {
            rotation = FIX16(360);
        }

    if (rotation >= FIX16(360))
        {
            rotation = FIX16(0);
        }*/

}

/************************************************************************
*                       !---convertToTable---!
* function type dynamic
* Return Type: unsigned 16 bit integer
* parameters: fix16 degrees
*
*           WARNING: This method is too inaccurate 32 bit math ops
*           return the wrong values I'm converting this method into
*           a table.
*
*
* Convert degrees to 1024 based radian look up table. The way Sin and
* cosine work is that they must be passed a integer ranging from 0 to
* 1024. Those numbers correspond to radians I however converted degrees
* to the table for instance 90° = 256 and 360° = 1024.
*
* OLD NOTES:
* I've found that there is some inaccuracies when computing larger 32 bit
* numbers but it is well within tolerances so I guess I can just leave it
* for now.
*
* If this method proves to be too taxing for the CPU I will replace
* this method with an array with the values needed for incrementing 9°
* every cycle.
************************************************************************/

u16 convertToTable(fix16 degrees)
{
    //I did have this set up to calculate the table position on its own
    //but for some reason 32 bit math keeps returning the wrong answers
    //after 270°

    //I have made a spread sheet with the appropriate values to compensate
    //and have incorporated them into the table array. If anyone else should
    //read this and see where my flaw is please contact me.

    /*u32 step1 = fix16ToInt(degrees) * 1000000;
    u32 step2 = step1 / 351562;

    u16 conversion = step2;

    //Display the conversion variable
    char rad[16];
    fix16ToStr(rotation, rad, 4);
    VDP_drawText(rad, 13, 16);

    return conversion;*/

    //Turn the degrees into an index
    u16 index = fix16ToInt(degrees) / 9;

    //This is for 9 degree transforms
    /*
    u16 table[41] = {
        0,   26,   51,  77, 102,
        128, 154, 179, 205, 230,
        256, 282, 307, 333, 358,
        384, 410, 435, 461, 486,
        512, 538, 563, 589, 614,
        640, 666, 691, 717, 742,
        768, 794, 819, 845, 870,
        896, 922, 947, 973, 998,
        1024
    };*/

    u16 table[41] = {
         17,  34,  51,  68,  85, 102,
        119, 137, 154, 171, 188, 205,
        222, 239, 256, 273, 290, 307,
        324, 341, 358, 375, 393, 410,
        427, 444, 461, 478, 495, 512,
        529, 546, 563, 580, 597, 614,
        631, 649, 666, 683, 700, 717,
        734, 751, 768, 785, 802, 819,
        836, 853, 870, 887, 905, 922,
        939, 956, 973, 990, 1007, 1024
    };
    return table[index];
}

/*************************************************************************
*                       !--- accelerateShip ---!
* function type: static
* Return Type: void
* parameters: none
*
* This function takes the degree that the ship is at and converts it to
* radians. calculates the trajectory of the ship based on acceleration and
* angle.
*
* This function modifies the velocity of the ship which is at position 62.
* I may need to switch all of these to FIX16 because of the additional
* processing power I would need to do 32 bit fixes.
*************************************************************************/

static void accelarateShip() {

    u16 test = convertToTable(rotation);

    //This must be fixed here to the proper functions
    velocity[62][0] = fix16Add((cosFix16(test) / 9), velocity[62][0]);
    velocity[62][1] = fix16Add(-(sinFix16(test) / 9), velocity[62][1]);

    //Debugging
    /*char rad[16];
    uint16ToStr(test, rad, 4);
    VDP_drawText(rad, 12, 16);*/

    //get rid of this later
    //velocity[62][0] = fix16Add(FIX16(0.1), velocity[62][0]);

    //make sure that the ship is not going too fast
    //X axis acceleration
    if (velocity[62][0] > FIX16(6))
    {
        velocity[62][0] = FIX16(6);
    }
    if (velocity[62][0] < FIX16(-6))
    {
        velocity[62][0] = FIX16(-6);
    }
    //Y axis acceleration
    if (velocity[62][1] > FIX16(6))
    {
        velocity[62][1] = FIX16(6);
    }
    if (velocity[62][1] < FIX16(-6))
    {
        velocity[62][1] = FIX16(-6);
    }
}

/*************************************************************************
*                        !--- fireBullets ---!
* function type: static
* Return Type: void
* Parameters: None
* Variables: unsigned 16 bit test
*            MODIFIES: indexBullets
*
* This function is called by the handleInput() function when the B or C
* buttons are pressed. Bullets can be created at position 32 - 61. Positions
* 0 - 31 are reserved for asteroids
*
* Spawn a bullet at the ships position. There can be no more than 30 bullets
* on screen at once. this function will wrap back to the beginning and
* overwrite old bullets.
*************************************************************************/

static void fireBullets()
{
    //Need to create a bullet at a vacant position.
    u16 test = convertToTable(rotation);

    positions[indexBullets][0] = fix16Add(positions[62][0], FIX16(8));
    positions[indexBullets][1] = fix16Add(positions[62][1], FIX16(0));
        //Set the velocity based on rotation
    velocity[indexBullets][0] = fix16Add((cosFix16(test) * FIX16(0.08)), velocity[62][0]);
    velocity[indexBullets][1] = fix16Add(-(sinFix16(test) * FIX16(0.08)), velocity[62][1]);

    //Make sure the bullets don't go to fast
    if (velocity[indexBullets][0] > FIX16(6))
    {
        velocity[indexBullets][0] = FIX16(6);
    }
    if (velocity[indexBullets][1] > FIX16(6))
    {
        velocity[indexBullets][1] = FIX16(6);
    }
    //Create the bullet at the proper position
    //Delete existing bullet sprites
    if (!sprites[indexBullets])
    {
        sprites[indexBullets] = SPR_addSprite(&bullet, fix16ToInt(positions[indexBullets][0]),
                                          fix16ToInt(positions[indexBullets][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    }

    indexBullets += 1;

    //Wrap the bullets
    if (indexBullets == 62)
    {
        indexBullets = 32;
    }

    //debugging show index
    char rad[16];
    uint16ToStr(indexBullets, rad, 2);
    VDP_drawText(rad, 14, 16);

}

/*************************************************************************
*                       !--- shipAnimation---!
* function type: static
* Return Type: void
* Parameters: None
*
* This function is called after handle input has been called in the main
* game loop. It updates the animation based on what angle the ship is
* resting at.
*
* The Animation is only 11 frames. But I have made it look like it has
* 44 frames by vertically and horizontally flipping the sprite. This
* saves us quite a bit of memory.
*************************************************************************/


static void shipAnimation()
{
    //Frame of the animation is based off of angle
    u16 frame = fix16ToInt(rotation) / 6;

    //Ship facing east
    if (rotation >= FIX16(0) && rotation <= FIX16(90))
    {
        frame = 10 - frame;
        SPR_setHFlip(sprites[62], FALSE);
        SPR_setVFlip(sprites[62], FALSE);
    }
    else if (rotation > FIX16(90) && rotation <= FIX16(180))
    {
        frame = frame - 10;
        SPR_setHFlip(sprites[62], TRUE);
        SPR_setVFlip(sprites[62], FALSE);
    }
    else if (rotation > FIX16(180) && rotation <= FIX16(270))
    {
        frame = 10 - (frame - 20);
        SPR_setHFlip(sprites[62], TRUE);
        SPR_setVFlip(sprites[62], TRUE);
    }
    else if (rotation > FIX16(270) && rotation < FIX16(360))
    {
        frame = (frame - 30);
        SPR_setHFlip(sprites[62], FALSE);
        SPR_setVFlip(sprites[62], TRUE);
    }

    char str2[16];
    uint16ToStr(frame, str2, 2);
    VDP_drawText(str2, 9, 12);
    SPR_setFrame(sprites[62], frame);
}

/***************************************************************
*                   !---update Physics---!
*
* Update Physics functions variables can be found in the main
* function. Just a quick note, if I need to do memory allocation
* use the MEM_alloc(sizeof()) command. To free use MEM_free().
* this will be a void it will just scroll a couple sprites for now.
*
* This function is going to have the responsibility of updating
* the positions of everything. After is has updated position it checks to see
* if there are any collisions. If so it will delete the sprite
* and set the X to 400 indicating it is dead.
***************************************************************/

static void updatePositions(){
    //index through the asteroids coordinates.
    u8 i;
    u8 j;
    for(i = 0; i < 64; i++)
    {
        if(positions[i][0] != FIX16(400))
        {
            //Screen rapping X+
            if (positions[i][0] > FIX16(302))
            {
                positions[i][0] = FIX16(-15);
            }
            //Screen Rapping X-
            if(positions[i][0] < FIX16(-15))
            {
                positions[i][0] = FIX16(302);
            }
            //Screen Rap Y+
            if (positions[i][1] > FIX16(206))
            {
                positions[i][1] = FIX16(-15);
            }
            //Screen Rap Y-
            if (positions[i][1] < FIX16(-15))
            {
                positions[i][1] = FIX16(206);
            }

            //This should be fixed to the fix16Add(f1, f2)

            positions[i][0] = fix16Add(positions[i][0], velocity[i][0]);
            positions[i][1] = fix16Add(positions[i][1], velocity[i][1]);
            SPR_setPosition(sprites[i], fix16ToInt(positions[i][0]), fix16ToInt(positions[i][1]));

            //if collide destroy asteroid and bullet
            //System will take the indexed asteroid i
            //Loop through all bullets on screen and see if any are hitting it
            for(j = i + 1; j < 64; j++)
            {
                if(positions[j][0] != FIX16(400))
                {
                    /*if (checkCollision(positions[i][0], positions[i][1], positions[j][0], positions[j][1]) == 1)
                    {
                        SPR_releaseSprite(sprites[i]);
                        SPR_releaseSprite(sprites[j]);
                        positions[i][0] = FIX16(400);
                        positions[j][0] = FIX16(400);
                    }*/
                }
            }
        }
    }
}

/***************************************************************
*                   !--Check Collision--!
*   This function is void. It will check if the sprites have
* have collided and it will release the sprites from memory
* will call the create asteroid function. Function will return
* a 1 when the asteroids touch for now.
***************************************************************/

u8 checkCollision(fix16 x, fix16 y, fix16 xTwo, fix16 yTwo)
{
    if((fix16Add(x, FIX16(28))) < xTwo || x > (fix16Add(xTwo, FIX16(28)))) return 0;
    if((fix16Add(y, FIX16(28))) < yTwo || y > (fix16Add(yTwo, FIX16(28)))) return 0;

    return 1;
}
