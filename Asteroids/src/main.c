#include <genesis.h>
//#include <timer.h> //Only for debug
#include <tools.h>
#include "sprite.h"
#include "gfx.h"

Sprite* sprites[64];
u16 ind;
//All Sprites Coordinates, Position, hit boxes and angles except the ship
fix16 positions[64][2];
fix16 velocity[64][2];
u8 indexBullets;
u8 hitBox[64];
u16 angles[64]; //These will only have values of 1 - 1024
u32 Score;
u8 numAsteroids;
u8 cooperative = 0;
s8 player1;
s8 player2;
s8 timer[2];

//Value of what time the bullets should die
u16 bulletLimit[14];
//Controller status values
s8 pressed;
u8 counter;

//Ship Rotation in degrees
fix16 rotation[2];

//Prototypes in order of which they are called.
static void titleScreen();
static void setupLevel(s16 level);
u8 gameLoop();
u8 gameOver();
static void addCounter();
static void handleInput();
u16 convertToTable(fix16 degrees);
static void accelarateShip(u8 shipIndex);
static void shipAnimation(u8 shipIndex);
static void explosion();
static void updatePositions();
static void fireBullets(u8 shipIndex);
u8 checkCollision(u8 i, u8 j);
static void displayScore();
static void checkRespawnShip();
static void clearScreen();
static void checkLevelOver();


/**************************************************************
*              !-- Create Asteroid --!
* takes the number of the asteroid hit deletes it. I'm filling
* this in later.
**************************************************************/

void createAsteroids(u8 hitAsteroid)
{
    u8 index;

    if (hitAsteroid < 32 && hitBox[hitAsteroid] != 4)
    {
        //Make the first split
        index = 0;
        //Loop to an empty position
        while (positions[index][0] != FIX16(400))
        {
            index += 1;
        }
        positions[index][0] = fix16Add(positions[hitAsteroid][0], FIX16(6));
        positions[index][1] = fix16Add(positions[hitAsteroid][1], FIX16(6));

        //set the angles based off hit asteroid
        if (angles[hitAsteroid] - 16 < 0)
        {
            angles[index] = (angles[hitAsteroid] - 16) + 1024;
        }
        else
        {
            angles[index] = angles[hitAsteroid] - 16;
        }

        //Set the velocity of the new asteroids
        velocity[index][0] = fix16Add((cosFix16(angles[index])), (fix16Div(velocity[hitAsteroid][0], FIX16(2))));
        velocity[index][1] = fix16Add(-(sinFix16(angles[index])), (fix16Div(velocity[hitAsteroid][1], FIX16(2))));

        //The only two variable that really determine what asteroid this is
        if (hitBox[hitAsteroid] == 16)
        {
            hitBox[index] = 8;
            sprites[index] = SPR_addSprite(&medium1, fix16ToInt(positions[index][0]), fix16ToInt(positions[index][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        }
        else
        {
            hitBox[index] = 4;
            sprites[index] = SPR_addSprite(&small1, fix16ToInt(positions[index][0]), fix16ToInt(positions[index][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        }

        //Make the second Split
        index = 0;
        //Loop to an empty position
        while (positions[index][0] != FIX16(400))
        {
            index += 1;
        }
        positions[index][0] = fix16Add(positions[hitAsteroid][0], FIX16(6));
        positions[index][1] = fix16Add(positions[hitAsteroid][1], FIX16(6));

        //set the angle based off hit asteroid
        if (angles[hitAsteroid] + 16 > 1024)
        {
            angles[index] = (angles[hitAsteroid] + 16) - 1024;
        }
        else
        {
           angles[index] = angles[hitAsteroid] + 16;
        }

        //Set the velocity of the new asteroids
        velocity[index][0] = fix16Add((cosFix16(angles[index])), (fix16Div(velocity[hitAsteroid][0], FIX16(2))));
        velocity[index][1] = fix16Add(-(sinFix16(angles[index])), (fix16Div(velocity[hitAsteroid][1], FIX16(2))));


        if (hitBox[hitAsteroid] == 16)
        {
            hitBox[index] = 8;
            sprites[index] = SPR_addSprite(&medium1, fix16ToInt(positions[index][0]), fix16ToInt(positions[index][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        }
        else
        {
            hitBox[index] = 4;
            sprites[index] = SPR_addSprite(&small1, fix16ToInt(positions[index][0]), fix16ToInt(positions[index][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        }
        numAsteroids += 2;
    }
}

/***************************************************************
* This is the Main Function I'm initializing the Video Chip
* I'm also going to keep all of variables in here maybe
* Main Game Loop is in main.
***************************************************************/

int main() {
    //These are variables its always best to use everything sparingly because there is not much
    //space in memory
    u16 palette[64];

    char str[16];

    // disable interrupt when accessing VDP
    SYS_disableInts();
    // initialization
    VDP_setScreenWidth320();

     // init sprites engine
    SPR_init(200, 384, 256);

    // set all palette to black
    VDP_setPaletteColors(0, (u16*) palette_black, 64);

    // VDP process done, we can re enable interrupts
    // We are re-enabling interrupts you can now draw sprites FOREVER until
    // reloading the background planes.
    SYS_enableInts();

    //I'm going to assign Tile User Index in the main function lets see if this
    //breaks everything
    ind = TILE_USERINDEX;

    //LevelSetup used to be here

    //Updating screens
    SPR_update();

    //Copy the colors into memory after updating the sprite engine
    memcpy(&palette[0], big1.palette->data, 16 * 2);

    //Fade in for kicks and giggles Fade in is 60 milliseconds it looks like or cycles
    VDP_fadeIn(0, (4 * 16) - 1, palette, 60, FALSE);
    //Apparently pointers are needed
    fix32ToStr(getFPS(), str, 2);

    while (1)
    {
        //Need to call the TitleScreen
        //titleScreen();

        //Call the first level for now
        setupLevel(0);
        while (gameOver() == 0) { gameLoop(); }
        clearScreen();
    }

	return (0);
}

/*****************************************************************************
*                           !---titleScreen---!
*
*
* This function displays the title of the game and it also controls the
* different options you can set. It can set Co-Operative mode as well as rate
* of fire and lives count.
*****************************************************************************/

void titleScreen()
{
    SPR_update();
    u8 displayed = 0;
    u16 value = JOY_readJoypad(JOY_1);
    //Nullified User INDEX because things might be breaking from multiple access points
    ind = TILE_USERINDEX;
    while (1)
    {
        if (displayed == 0)
        {
            SYS_disableInts();
            //display image
            VDP_drawImageEx(PLAN_A, &bga_title, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 10, 12, FALSE, TRUE);
            //Tileset was indexing with bga_gameover causing a bug which has been fixed.
            ind += bga_title.tileset->numTile;
            SYS_enableInts();
            displayed = 1;
        }
        //VDP_drawText(test, 15, 16);
        value = JOY_readJoypad(JOY_1);
        if (value & BUTTON_DOWN/*value & BUTTON_START*/)
        {
            break;
        }
    }
    clearScreen();
}



/*************************************************************************
*                           !---gameLoop---!
*
* This is more or less a list of all the different functions needed to
* make the game run.
**************************************************************************/

u8 gameLoop()
{
    //Set tile index to the beginning see what it does
    ind = TILE_USERINDEX;

    //read input
    handleInput();

    //move sprite
    updatePositions();
    checkRespawnShip();

    //check if asteroids are dead
    checkLevelOver();

    //Update sprites again
    SPR_update();

    //wait for screen refresh
    VDP_waitVSync();

    //Display Timer check crashes
    //fix32ToStr(getTimeAsFix32(getTick()), str, 2);
    //VDP_drawText(str, 10, 13);

    //The counter controls bullet rapid fire and bullet
    //living time
    addCounter();

    return 0;
}

/************************************************************************************
*                           !---gameOver---!
* Variables Modified:
*       s8 timer
*
*       The timer variable is used as a flag
*
* Displays a Game Over Screen when the player has no more lives.
************************************************************************************/
u8 gameOver()
{
    //This is now being done in the main game loop. As far as I know this is being set
    //to the beginning plus 16 tiles over from the system beginning
    //ind = TILE_USERINDEX;

    if (player1 == 0 && positions[46][0] == FIX16(400) && timer[0] != -12)
    {
        displayScore();
        SYS_disableInts();
        //display image
        VDP_drawImageEx(PLAN_A, &bga_gameover, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 16, 13, FALSE, TRUE);
        ind += bga_gameover.tileset->numTile;
        SYS_enableInts();
        //This is a signal to not run this code more than once
        //writing stuff to the screen over and over again is expensive
        timer[0] = -12;
        //update score
        //displayScore();
        return 0;
    }
    //Get the value from the controller
    u16 value = JOY_readJoypad(JOY_1);
    if (value & BUTTON_START && player1 == 0 && positions[46][0] == FIX16(400))
    {
        return 1;
    }

    return 0;
}

/************************************************************************
*                       !---setupLevel---!
* Return Type: void
* Parameters: u16 level
*
* Variables:
*   u8 index  - Used for looping through the sprites
*   u16 miscx - for assigning random X positions
*   u16 miscy - for assigning random Y positions
*
* startGame() function will take a level and set up 4 to 8 asteroids
* starting at level 1. It will use a random number generator to place
* the asteroids on screen but it will not place any asteroids inside the
* ship starting zone which will be a 70 x 70 square of pixels.
*
* It will also assign a velocity for the asteroids to travel at which
* will also be randomly generated.
************************************************************************/
static void setupLevel(s16 level) //this used to be a static function
{
    u8 index;
    //Misc stands for miscellaneous I know its weird
    u16 miscx;
    u16 miscy;

    if (level == 0)
    {
        //This needs to happen on level 0
        for (index = 0; index < 64; index++)
        {
            positions[index][0] = FIX16(400);
            hitBox[index] = 0;
            angles[index] = 0;
        }

        //Set default coordinates and set up game
        //default rotation
        indexBullets = 32;
        counter = 0;
        pressed = -1;
    }
    //One For Loop to set up them all
    //Need to figure out how to use random function
    for (index = 0; index < 4; index++)
    {
        //Random Value for X
        miscx = random() % 1000;
        while (miscx < 0 || miscx > 400)
        {
            miscx = random() % 1000;
        }
        miscy = random() % 1000;
        //Random Value for position Y
        while (miscy < 0 || miscy > 400)
        {
            miscy = random() % 1000;
        }
        positions[index][0] = intToFix16(miscx);
        positions[index][1] = intToFix16(miscy);

        //Setting the HitBox of the asteroids
        hitBox[index] = 16;

        //Random angle must be made
        angles[index] = random() % 10000;
        while (angles[index] > 1024)
        {
            angles[index] = random() % 10000;
        }

        //Modifying the angle stuff
        velocity[index][0] = cosFix16(angles[index]) / 2;
        velocity[index][1] = -(sinFix16(angles[index]) / 2);

        //Add the Big Asteroids
        sprites[index] = SPR_addSprite(&big1, fix16ToInt(positions[index][0]), fix16ToInt(positions[index][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        numAsteroids++;
    }

    //Initialize the Ship at level 0
    //Setting ship position
    if (level == 0 && cooperative == 0)
    {
        //Player 1
        rotation[0] = 0;
        positions[46][0] = FIX16(160);
        positions[46][1] = FIX16(112);
        velocity[46][0] = FIX16(0);
        velocity[46][1] = FIX16(0);
        sprites[46] = SPR_addSprite(&ship_sprite, fix16ToInt(positions[46][0]), fix16ToInt(positions[46][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        //Default ship resting
        shipAnimation(46);
        hitBox[46] = 8;
        player1 = 3;
        Score = 0;
        displayScore();
        timer[0] = -1;
    }
    if (level == 0 && cooperative == 1)
    {
        //Player 1
        rotation[0] = 0;
        positions[46][0] = FIX16(160);
        positions[46][1] = FIX16(112);
        velocity[46][0] = FIX16(0);
        velocity[46][1] = FIX16(0);
        sprites[46] = SPR_addSprite(&ship_sprite, fix16ToInt(positions[46][0]), fix16ToInt(positions[46][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        //Default ship resting
        shipAnimation(46);
        hitBox[46] = 8;
        player1 = 3;

        //Player 2
        rotation[1] = 0;
        positions[47][0] = FIX16(160);
        positions[47][1] = FIX16(112);
        velocity[47][0] = FIX16(0);
        velocity[47][1] = FIX16(0);
        sprites[47] = SPR_addSprite(&ship_sprite, fix16ToInt(positions[47][0]), fix16ToInt(positions[47][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        //Default ship resting
        shipAnimation(47);
        hitBox[47] = 8;
        player2 = 3;

        //Initial Game Conditions
        Score = 0;
        displayScore();
        timer[0] = -1;
        timer[1] = -1;
    }
}

/************************************************************************
*                       !---counter---!
* Return type: void
* No Parameters
*
* Variables:
* MODIFIED:
* unsigned 8 bit counter
*
* This is for the handle input function and the ships rapid fire
* function increments counter and raps it every 60 ticks.
*
* Could have used the timer functions included in <timer.h> but using a
* counter requires less overhead.
************************************************************************/

static void addCounter()
{
    counter ++;

    if (counter == 40)
    {
        counter = 0;
    }

    //debugging
    char counterDisplay[16];
    uint16ToStr(counter, counterDisplay, 4);
    VDP_drawText(counterDisplay, 15, 16);
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
* This function Modifies the rotation variable by a factor of 6
* Function also modifies ship velocity at [46].
*
* The only inputs I need to worry about here are the DPAD and the A, B
* and C buttons all of which will fire a bullet. Calls shipAnimation()
* whenever the ship is rotated.
*
************************************************************************/

static void handleInput()
{
    //Get the value from the controller
    u16 value =  JOY_readJoypad(JOY_1);
    u16 value2 = JOY_readJoypad(JOY_2);

    //Up button does nothing for now will control thrust
    if (value & BUTTON_UP)
    {
        accelarateShip(46);
    }

    if (value & BUTTON_DOWN)
    {

    }

    //Increment angle by 3 and then integer math
    if (value & BUTTON_LEFT)
    {
        rotation[0] = fix16Add(rotation[0], FIX16(6));
        //Make sure the rotation is not less than 0
        if (rotation[0] >= FIX16(360))
        {
            rotation[0] = FIX16(0);
        }
        //Now update the ship animation.
        shipAnimation(46);
    }

    else if (value & BUTTON_RIGHT)
    {
        rotation[0] = fix16Add(rotation[0], FIX16(-6));
        //Make sure degrees does not exceed 360
        if (rotation[0] < FIX16(0))
        {
            rotation[0] = FIX16(354);
        }
        //Now we update the animation
        shipAnimation(46);
    }


    if (value & BUTTON_A)
    {
        if (pressed == -1)
        {
            fireBullets(46);
            pressed = counter % 20; //counter % 20
        }

        else if ((counter % 20) == pressed)
        {
            fireBullets(46);
        }
    }

    else
    {
        pressed = -1;
    }

    //If Co-Operative game play is selected check for second controller input
    if (cooperative == 1)
    {
        if (value2 & BUTTON_UP)
        {
            accelarateShip(47);
        }

        if (value2 & BUTTON_DOWN)
        {

        }

        //Increment angle by 3 and then integer math
        if (value2 & BUTTON_LEFT)
        {
            rotation[1] = fix16Add(rotation[1], FIX16(6));
            //Make sure the rotation is not less than 0
            if (rotation[1] >= FIX16(360))
            {
                rotation[1] = FIX16(0);
            }
            //Now update the ship animation.
            shipAnimation(47);
        }

        else if (value2 & BUTTON_RIGHT)
        {
            rotation[1] = fix16Add(rotation[1], FIX16(-6));
            //Make sure degrees does not exceed 360
            if (rotation[1] < FIX16(0))
            {
                rotation[1] = FIX16(354);
            }
            //Now we update the animation
            shipAnimation(47);
        }


        if (value2 & BUTTON_A)
        {
            if (pressed == -1)
            {
                fireBullets(47);
                pressed = counter % 20; //counter % 20
            }

            else if ((counter % 20) == pressed)
            {
                fireBullets(47);
            }
        }

        else
        {
            pressed = -1;
        }
    }
    //Display the pressed variable
    /*char rad[16];
    fix16ToStr(pressed, rad, 2);
    VDP_drawText(rad, 14, 16);*/
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
    //Turn the degrees into an index
    u16 index = fix16ToInt(degrees) / 6;

    u16 table[61] = {
        0,    17,  34,  51,  68,  85,
        102, 119, 137, 154, 171, 188,
        205, 222, 239, 256, 273, 290,
        307, 324, 341, 358, 375, 393,
        410, 427, 444, 461, 478, 495,
        512, 529, 546, 563, 580, 597,
        614, 631, 649, 666, 683, 700,
        717, 734, 751, 768, 785, 802,
        819, 836, 853, 870, 887, 905,
        922, 939, 956, 973, 990, 1007,
        1024
    };
    return table[index];
}

/****************************************************************************
*                       !--- accelerateShip ---!
* function type: static
* Return Type: void
* parameters: none
*
* This function takes the degree that the ship is at and converts it to
* radians. calculates the trajectory of the ship based on acceleration and
* angle.
*
* This function modifies the velocity of the ship which is at position 46.
* I may need to switch all of these to FIX16 because of the additional
* processing power I would need to do 32 bit fixes.
****************************************************************************/

static void accelarateShip(u8 shipIndex) {

    //Calculate Total Velocity
    fix16 vx = velocity[shipIndex][0];
    fix16 vy = velocity[shipIndex][1];
    u16 test = convertToTable(rotation[(shipIndex - 46)]);

    //This isn't accelerating properly trying again
    velocity[shipIndex][0] = fix16Add((cosFix16(test) / 9), velocity[shipIndex][0]);
    velocity[shipIndex][1] = fix16Add(-(sinFix16(test) / 9), velocity[shipIndex][1]);

    //Debugging
    /*char rad[16];
    uint16ToStr(test, rad, 4);
    VDP_drawText(rad, 12, 16);*/

    //make sure that the ship is not going too fast
    //We are going to use Pythagora's theorem to calculate total velocity... roughly
    //I'm not going to find square root of the total velocity because it will put too
    //much stress on the genesis and slow the game down.
    if (FIX16(10) < fix16Add(fix16Mul(velocity[shipIndex][0],velocity[shipIndex][0]), fix16Mul(velocity[shipIndex][1],velocity[shipIndex][1])))
    {
        velocity[shipIndex][0] = vx;
        velocity[shipIndex][1] = vy;
    }
}

/******************************************************************************
*                        !--- fireBullets ---!
* function type: static
* Return Type: void
* Parameters: None
* Variables: unsigned 16 bit test
*            MODIFIES: indexBullets
*
* This function is called by the handleInput() function when the B or C
* buttons are pressed. Bullets can be created at position 32 - 46. Positions
* 0 - 31 are reserved for asteroids
*
* Spawn a bullet at the ships position. There can be no more than 30 bullets
* on screen at once. this function will wrap back to the beginning and
* overwrite old bullets.
****************************************************************************/

static void fireBullets(u8 shipIndex)
{
    //If the ship is dead don't fire the bullets
    if (positions[shipIndex][0] != FIX16(400))
    {
        //Create the bullet at the proper position
        if (positions[indexBullets][0] == FIX16(400))  //!sprites[indexBullets]
        {
            //Get the position of the ship spawn bullet at position
            positions[indexBullets][0] = fix16Add(positions[shipIndex][0], FIX16(8));
            positions[indexBullets][1] = fix16Add(positions[shipIndex][1], FIX16(8));

            //Make the sprite
            sprites[indexBullets] = SPR_addSprite(&bullet, fix16ToInt(positions[indexBullets][0]),
                                              fix16ToInt(positions[indexBullets][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        }
        else
        {
            //Get the position of the ship spawn bullet at position
            positions[indexBullets][0] = fix16Add(positions[shipIndex][0], FIX16(8));
            positions[indexBullets][1] = fix16Add(positions[shipIndex][1], FIX16(0));
        }

        //Set the velocity based on rotation
        //Need to create a bullet at a vacant position.
        u16 test = convertToTable(rotation[shipIndex - 46]);
        velocity[indexBullets][0] = fix16Add((cosFix16(test) * FIX16(0.08)), velocity[shipIndex][0]);
        velocity[indexBullets][1] = fix16Add(-(sinFix16(test) * FIX16(0.08)), velocity[shipIndex][1]);

        //Make sure the bullets don't go too fast
        if (velocity[indexBullets][0] > FIX16(6))
        {
            velocity[indexBullets][0] = FIX16(6);
        }
        if (velocity[indexBullets][1] > FIX16(6))
        {
            velocity[indexBullets][1] = FIX16(6);
        }

        //Bullet Limits control the distance at which the bullets can go
        //before they die
        u8 temp;

        if (counter == 0)
        {
            temp = 39;
        }
        else
        {
            temp = (counter - 1);
        }

        bulletLimit[(indexBullets - 32)] = temp;

        /*
        //debugging show index
        char rad[16];
        uint16ToStr(bulletLimit[indexBullets - 32], rad, 3);
        VDP_drawText(rad, 14, 17);
        */

        //increment the index by 1
        indexBullets += 1;

        //Wrap the bullets only 14 bullets on screen
        if (indexBullets > 45)
        {
            indexBullets = 32;
        }
    }
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


static void shipAnimation(u8 shipIndex)
{
    //Frame of the animation is based off of angle
    u16 frame = fix16ToInt(rotation[shipIndex - 46]) / 6;

    //Ship facing east
    if (rotation[shipIndex - 46] >= FIX16(0) && rotation[shipIndex - 46] <= FIX16(90) && positions[shipIndex][0] != FIX16(400))
    {
        frame = 15 - frame;
        SPR_setHFlip(sprites[shipIndex], FALSE);
        SPR_setVFlip(sprites[shipIndex], FALSE);
    }
    else if (rotation[shipIndex - 46] > FIX16(90) && rotation[shipIndex - 46] <= FIX16(180) && positions[shipIndex][0] != FIX16(400))
    {
        frame = frame - 15;
        SPR_setHFlip(sprites[shipIndex], TRUE);
        SPR_setVFlip(sprites[shipIndex], FALSE);
    }
    else if (rotation[shipIndex - 46] > FIX16(180) && rotation[shipIndex - 46] <= FIX16(270) && positions[shipIndex][0] != FIX16(400))
    {
        frame = 15 - (frame - 30);
        SPR_setHFlip(sprites[shipIndex], TRUE);
        SPR_setVFlip(sprites[shipIndex], TRUE);
    }
    else if (rotation[shipIndex - 46] > FIX16(270) && rotation[shipIndex - 46] < FIX16(360) && positions[shipIndex][0] != FIX16(400))
    {
        frame = (frame - 45);
        SPR_setHFlip(sprites[shipIndex], FALSE);
        SPR_setVFlip(sprites[shipIndex], TRUE);
    }

    /*
    //Debugging Show Index of Frame used
    char str2[16];
    uint16ToStr(frame, str2, 2);
    VDP_drawText(str2, 9, 12);
    */
    if (positions[shipIndex][0] != FIX16(400))
    {
        SPR_setFrame(sprites[shipIndex], frame);
    }
}
 /**********************************************************************
 *                  !---explosion---!
 * \brief the explosion animation should only be 16 frames and it will
 * play for everything. Ship explosions asteroids everything just like
 * the original arcade game.
 * \param none
 * \return void
 *
 *********************************************************************/

 static void explosion()
 {
    u16 frame = 0;

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


            //Check to see if the bullets need to die
            //due to how long they've been on screen
            //not working

            if (i >= 32 && i <= 45)
            {
                if(bulletLimit[(i - 32)] == counter && positions[i][0] != FIX16(400)) //got rid of 400 check
                {
                    SPR_releaseSprite(sprites[i]);
                    positions[i][0] = FIX16(400);
                    /*
                    //debugging for showing the Index of deleted bullet
                    char look[16];
                    uint16ToStr((i - 32), look, 3);
                    VDP_drawText(look, 14, 18);
                    */
                }
            }

            //if collide destroy asteroid and bullet
            //System will take the indexed asteroid i
            //Loop through all bullets on screen and see if any are hitting it
            //only check for collisions with asteroids which is 0 - 31
            //This might be a bad idea... It could mess up the amount of time
            //the loop takes to complete.
            if (i < 32)
            {
                //j is bullets
                for(j = 32; j < 47; j++)
                {
                    if(positions[j][0] != FIX16(400))
                    {
                        if (checkCollision(i, j) == 1)
                        {
                            //Add to the score when certain asteroids are hit
                            if(hitBox[i] == 16)
                            {
                                Score += 20;
                            }
                            else if(hitBox[i] == 8)
                            {
                                Score += 50;
                            }
                            else if(hitBox[i] == 4)
                            {
                                Score += 100;
                            }
                            SPR_releaseSprite(sprites[i]);
                            SPR_releaseSprite(sprites[j]);
                            createAsteroids(i);
                            //play asteroid destruction animation
                            explosion();
                            positions[i][0] = FIX16(400);
                            positions[j][0] = FIX16(400);
                            numAsteroids--;
                            //sprites[i] = NULL;
                            displayScore();
                        }
                    }
                }
            }


            //We are checking to see if the asteroid is dead again because it
            //could have died while we were doing collision.
            if (positions[i][0] != FIX16(400))
            {
                positions[i][0] = fix16Add(positions[i][0], velocity[i][0]);
                positions[i][1] = fix16Add(positions[i][1], velocity[i][1]);
                SPR_setPosition(sprites[i], fix16ToInt(positions[i][0]), fix16ToInt(positions[i][1]));
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

u8 checkCollision(u8 i, u8 j)
{
    if (i < 32)
    {
        //10 used to be 15
        /*if (abs(fix16Sub(positions[j][0], fix16Add(positions[i][0], intToFix16(hitBox[i])))) < FIX16(15) &&
            abs(fix16Sub(positions[j][1], fix16Add(positions[i][1], intToFix16(hitBox[i])))) < FIX16(15))
            return 1;*/

        if (abs(fix16Sub(fix16Add(positions[j][0], intToFix16(hitBox[j])), fix16Add(positions[i][0], intToFix16(hitBox[i])))) < FIX16(hitBox[i]) &&
            abs(fix16Sub(fix16Add(positions[j][1], intToFix16(hitBox[j])), fix16Add(positions[i][1], intToFix16(hitBox[i])))) < FIX16(hitBox[i]))
            return 1;
    }
    else
    {
        return 0;
    }
    return 0;
}

/*******************************************************************************
*                           !--displayScore--!
*
* Variables:
*       u32 temp
*       array of image pointers
*
*   This displays the score it uses an array of images to display the score in
*   the proper style.
********************************************************************************/
static void displayScore()
{
    // load background changing TILE_USERINDEX to TILE_SYSTEMINDEX
    //ind = TILE_USERINDEX; moving this to beginning of cycle going to see what it does.

    //Draw the number to plane A only when score changes

    SYS_disableInts();
    //Must use if statements for number displays

    //debugging for showing the Score of deleted bullet
                    char look[16];
                    uintToStr(Score, look, 3);
                    VDP_drawText(look, 0, 6);

    //000,000,001
    VDP_drawImageEx(PLAN_A, images[(Score % 10)], TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 10, 0, FALSE, TRUE);
    ind += images[Score % 10]->tileset->numTile; // *(*(images[counter % 10]).tileset).numTile;
    //000,000,010 Trying something really stupid
    VDP_drawImageEx(PLAN_A, images[((Score / 10) % 10)], TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 9, 0, FALSE, TRUE);
    ind += images[((Score / 10) % 10)]->tileset->numTile;
    //000,000,100
    VDP_drawImageEx(PLAN_A, images[((Score / 100) % 10)], TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 8, 0, FALSE, TRUE);
    ind += images[((Score / 100) % 10)]->tileset->numTile;
    //000,001,000
    if (Score >= 1000)
    {
        VDP_drawImageEx(PLAN_A, images[((Score / 1000) % 10)], TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 7, 0, FALSE, TRUE);
        ind += images[((Score / 1000) % 10)]->tileset->numTile;
    }
    //000,010,000
    if (Score >= 10000)
    {
        VDP_drawImageEx(PLAN_A, images[((Score / 10000) % 10)], TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 6, 0, FALSE, TRUE);
        ind += images[((Score / 10000) % 10)]->tileset->numTile;
    }
    //000,100,000
    if (Score >= 100000)
    {
        VDP_drawImageEx(PLAN_A, images[((Score / 100000) % 10)], TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 5, 0, FALSE, TRUE);
        ind += images[((Score / 100000) % 10)]->tileset->numTile;
    }
    //001,000,000
    if (Score >= 1000000)
    {
        VDP_drawImageEx(PLAN_A, images[((Score / 1000000) % 10)], TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 4, 0, FALSE, TRUE);
        ind += images[((Score / 1000000) % 10)]->tileset->numTile;
    }
    SYS_enableInts();
}
/*********************************************************************************
*                               !---checkRespawnShip---!
* Variables Modified:
*   s8 timer;
*
*       Timer is used in this function to allow a certain amount of time to pass
*       before the ship is re-spawned. when the
*
* This function will check if their are asteroids in the play field and then it
* will put the ship back on screen.
*
* Wait until the counter makes a full cycle before re-spawning the ship
**********************************************************************************/

static void checkRespawnShip()
{
    //If player 1 is dead start the countdown to re-spawn the player
    if (positions[46][0] == FIX16(400) && timer[0] == -1)
    {
        if (counter == 0)
        {
            timer[0] = 39;
        }
        else
        {
            timer[0] = (counter - 1);
        }
    }
    if (positions[47][0] == FIX16(400) && cooperative == 1 && timer[1] == -1)
    {
        if (counter == 0)
        {
            timer[1] = 39;
        }
        else
        {
            timer[1] = (counter - 1);
        }
    }
    else if (timer[0] == counter && player1 != 0 && positions[46][0] == FIX16(400))
    {
        rotation[0] = 0;
        positions[46][0] = FIX16(160);
        positions[46][1] = FIX16(112);
        velocity[46][0] = 0;
        velocity[46][1] = 0;
        sprites[46] = SPR_addSprite(&ship_sprite, fix16ToInt(positions[46][0]), fix16ToInt(positions[46][1]), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        //Default ship resting
        shipAnimation(46);
        hitBox[46] = 8;
        player1--;
        timer[0] = -1;
    }
}

/**********************************************************************************
*                             !---checkLevelOver---!
*
*Variables:
*   NONE
*
* This will check the numAsteroids flag at the end of each game loop. If all
* asteroids are dead the setupLevel() function will be called. It will pass in the
* level and it will set up the level accordingly.
**********************************************************************************/
static void checkLevelOver()
{
    if (numAsteroids == 0)
    {
        setupLevel(1);
    }
}

/********************************************************************************
* Destroy all objects on screen for level reset
*********************************************************************************/
static void clearScreen()
{
    VDP_clearPlan(PLAN_A, FALSE);
    SPR_reset();
}
