#include <genesis.h>
//#include <timer.h> //Only for debug
#include <tools.h>
#include "sprite.h"
#include "gfx.h"

/****************************************************************************
* FILE: main.c
* Author: Alexander Bluhm
* Game Title: MegaStroids
* Code Version: 2.0.0
*
* This is a Major rewrite of this Project. The introduction of structs has
* triggered a major rewrite of the code all in the name of efficiency and
* maintainability. I want to add more features to this game and it requires
* that the code be more dynamic. Also focusing on removing the parameters
* from as many functions as possible making code lean without sacrificing
* performance.
****************************************************************************/
//refactoring the code to be a bit more organized
typedef struct {
    Sprite* sprite;
    fix16 x;
    fix16 y;
    fix16 velX;
    fix16 velY;
    u8 hitBox;
    u8 isAlive;
    u16 angle;
}Entity;

typedef struct {
    Entity coord;
    u8 bulletLimit;
}Bullet;

typedef struct {
    Entity coord;
    u16 bulletLimit;
    u16 lives;
    s8 timer;
    u8 bulletIndex;
    //this will hold degrees instead of an integer index for the
    //sake of making the ship rotate at a slower pace. I may remove
    //this in a later update if I can it's too janky.
    fix16 rotation;
}Ship;

u16 ind; //Tile Index for loading information to VDP

// GNU GCC compiler passes parameters as 32 bit values which saturates the
// 16-bit bus and therefore slows down the game code. This isn't a problem
// for passing two 16 bit values in a function call as the compiler lumps it
// together but if you want to pass an 8 bit index it is really inefficient.
// Therefore I am adding a master index for all objects in the game. I am also
// adding a 8 bit generic parameter for 8 bit integers.
u8 byteParam1;
u8 byteParam2;

//these parameters take up two words in memory
u16 wordParam;

//Making a enumerator for readability as to what object type is being manipulated
enum objectType {
    rockType,
    bulletType,
    shipType
};

u32 Score;
u8 numAsteroids;

u8 cooperative = 0;
u8 numPlayers = 1;

//Where the Asteroids will live
Entity rocks[32];

//where the bullets will live
Bullet bullets[32];

//where the ships will live
Ship ships[2];

//Value of how many bullets allowed per player
u8 playerAmmunition;

//Controller status values
s8 pressed;
u8 counter;

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
u8 checkCollision(u8 index1, u8 objectType1, u8 index2, u8 objectType2);
static void displayScore();
static void checkRespawnShip();
static void clearScreen();
static void checkLevelOver();

const Image *images[10] = {
    &bga_0,
    &bga_1,
    &bga_2,
    &bga_3,
    &bga_4,
    &bga_5,
    &bga_6,
    &bga_7,
    &bga_8,
    &bga_9
};


/**************************************************************
*              !-- Create Asteroid --!
* takes the number of the asteroid hit deletes it. I'm filling
* this in later.
**************************************************************/

void createAsteroids(u8 hitAsteroid)
{
    //angle Rotation for new rocks when they split
    s16 angleDeflection = -16;
    //index for looping through the rocks twice
    u8 j = 0;
    u8 index;
    if (rocks[hitAsteroid].hitBox != 4)
    {
        while (j < 2)
        {
            //Make the first split from a big rock to two medium sized rocks
            index = 0;
            //Loop to an empty position
            while (rocks[index].isAlive == 0)
            {
                index += 1;
            }
            rocks[index].x = fix16Add(rocks[hitAsteroid].x, FIX16(6));
            rocks[index].y = fix16Add(rocks[hitAsteroid].y, FIX16(6));

            //set the angles based off hit asteroid
            if (rocks[hitAsteroid].angle + angleDeflection < 0)
            {
                rocks[index].angle = (rocks[hitAsteroid].angle + angleDeflection) + 1024;
            }
            if (rocks[hitAsteroid].angle + angleDeflection > 1024)
            {
                rocks[index].angle = (rocks[hitAsteroid].angle + angleDeflection) - 1024;
            }
            else
            {
                rocks[index].angle = rocks[hitAsteroid].angle + angleDeflection;
            }

            //Set the velocity of the new asteroids
            rocks[index].velX = fix16Add((cosFix16(rocks[index].angle)), (fix16Div(rocks[hitAsteroid].velX, FIX16(2))));
            rocks[index].velY = fix16Add(-(sinFix16(rocks[index].angle)), (fix16Div(rocks[hitAsteroid].velY, FIX16(2))));

            //The only two variables that really determine what asteroid this is is the hit box
            if (rocks[hitAsteroid].hitBox == 16)
            {
                rocks[index].hitBox = 8;
                rocks[index].sprite = SPR_addSprite(&medium1, fix16ToInt(rocks[index].x), fix16ToInt(rocks[index].y), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
            }
            else
            {
                rocks[index].hitBox = 4;
                rocks[index].sprite = SPR_addSprite(&small1, fix16ToInt(rocks[index].x), fix16ToInt(rocks[index].y), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
            }
            //increment to the next rock
            j += 1;
            angleDeflection = 16;
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
    SPR_init();

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

    if (ships[0].lives == 0 && ships[0].coord.isAlive == 0 && ships[0].timer != -12)
    {
        displayScore();
        SYS_disableInts();
        //display image
        VDP_drawImageEx(PLAN_A, &bga_gameover, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 16, 13, FALSE, TRUE);
        ind += bga_gameover.tileset->numTile;
        SYS_enableInts();
        //This is a signal to not run this code more than once
        //writing stuff to the screen over and over again is expensive
        ships[0].timer = -12;
        //update score
        //displayScore();
        return 0;
    }
    //Get the value from the controller
    u16 value = JOY_readJoypad(JOY_1);
    if (value & BUTTON_START && ships[0].lives == 0 && ships[0].coord.isAlive == 0)
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
static void setupLevel(s16 level) //I might remove this parameter later because it bugs me
{
    u8 index;
    //Misc stands for miscellaneous I know its weird
    u16 miscx;
    u16 miscy;

    switch(numPlayers) {
    case 1 :
        playerAmmunition = 16;
        break;
    case 2 :
        playerAmmunition = 16;
        break;
    case 3 :
        playerAmmunition = 10;
        break;
    case 4 :
        playerAmmunition = 8;
        break;
    }

    switch(level) {
    case 0 :
        numAsteroids = 4;
        break;
    case 1 :
        numAsteroids = 6;
        break;
    case 2 :
        numAsteroids = 8;
        break;
    default :
        numAsteroids = 8;
        break;
    }

    if (level == 0)
    {
        //Initialize all rocks to dead
        for (index = 0; index < 32; index++)
        {
            rocks[index].isAlive = 0;
            rocks[index].hitBox = 0;
            rocks[index].angle = 0;
        }

        //Initialize all bullets to dead
        for (index = 0; index < 32; index++)
        {
            bullets[index].coord.isAlive = 0;
            bullets[index].coord.hitBox = 0;
            bullets[index].coord.angle = 0;
        }

        //Set default coordinates and set up game
        //default rotation
        counter = 0;
        pressed = -1;

        //Initialize the Ship at level 0
        //Setting ship position
        if (cooperative == 0)
        {
            //Player 1
            ships[0].coord.angle = 0;
            ships[0].coord.x = FIX16(160);
            ships[0].coord.y = FIX16(112);
            ships[0].coord.velX = FIX16(0);
            ships[0].coord.velY = FIX16(0);
            ships[0].coord.sprite = SPR_addSprite(&ship_sprite, fix16ToInt(ships[0].coord.x), fix16ToInt(ships[0].coord.y), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
            //Default ship resting
            shipAnimation(0);
            ships[0].coord.hitBox = 8;
            ships[0].coord.isAlive = 1;
            ships[0].lives = 3;
            ships[0].bulletIndex = 0;
            Score = 0;
            ships[0].timer = -1;
        }
    }
    displayScore();

    //One For Loop to set up all rocks
    //In this case we are setting up 4 rocks
    index = 0;
    do {
        //Random Value for X the while loop is to check
        //if we have an appropriate value that will fit
        //in the play space
        miscx = random() % 1000;
        while (miscx < 0 || miscx > 400)
        {
            miscx = random() % 1000;
        }

        //Do the same for the Y value
        miscy = random() % 1000;
        //Random Value for position Y
        while (miscy < 0 || miscy > 400)
        {
            miscy = random() % 1000;
        }
        //set the positions of the randomly generated coordinates
        //to 1 of the four rocks in the first level
        rocks[index].x = intToFix16(miscx);
        rocks[index].y = intToFix16(miscy);

        //Setting the HitBox of the Big asteroids
        rocks[index].hitBox = 16;

        //Set the rocks to living
        rocks[index].isAlive = 1;

        //Random angle must be made
        rocks[index].angle = random() % 10000;
        while (rocks[index].angle > 1024)
        {
            rocks[index].angle = random() % 10000;
        }

        //Modifying the angle stuff
        rocks[index].velX = cosFix16(rocks[index].angle) / 2;
        rocks[index].velY = -(sinFix16(rocks[index].angle) / 2);

        //Add the Big Asteroid Sprite
        rocks[index].sprite = SPR_addSprite(&big1, fix16ToInt(rocks[index].x), fix16ToInt(rocks[index].y), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        numAsteroids++;
        index++;
    } while(index < 4); //This needs to be replaced with level incrementer
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
    uintToStr(counter, counterDisplay, 4);
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
        accelarateShip(0);
    }

    if (value & BUTTON_DOWN)
    {

    }

    //Increment angle by 3 and then integer math
    if (value & BUTTON_LEFT)
    {
        ships[0].rotation = fix16Add(ships[0].rotation, FIX16(6));
        //Make sure the rotation is not less than 0
        if (ships[0].rotation >= FIX16(360))
        {
            ships[0].rotation = FIX16(0);
        }
        //Now update the ship animation.
        shipAnimation(46);
    }

    else if (value & BUTTON_RIGHT)
    {
        ships[0].rotation = fix16Add(ships[0].rotation, FIX16(-6));
        //Make sure degrees does not exceed 360
        if (ships[0].rotation < FIX16(0))
        {
            ships[0].rotation = FIX16(354);
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
            ships[1].rotation = fix16Add(ships[1].rotation, FIX16(6));
            //Make sure the rotation is not less than 0
            if (ships[1].rotation >= FIX16(360))
            {
                ships[1].rotation = FIX16(0);
            }
            //Now update the ship animation.
            shipAnimation(47);
        }

        else if (value2 & BUTTON_RIGHT)
        {
            ships[1].rotation = fix16Add(ships[1].rotation, FIX16(-6));
            //Make sure degrees does not exceed 360
            if (ships[1].rotation < FIX16(0))
            {
                ships[1].rotation = FIX16(354);
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
* to the table for instance 90� = 256 and 360� = 1024.
*
* OLD NOTES:
* I've found that there is some inaccuracies when computing larger 32 bit
* numbers but it is well within tolerances so I guess I can just leave it
* for now.
*
* If this method proves to be too taxing for the CPU I will replace
* this method with an array with the values needed for incrementing 9�
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
    fix16 vx = ships[shipIndex].coord.velX;
    fix16 vy = ships[shipIndex].coord.velY;
    u16 test = convertToTable(ships[shipIndex].rotation);

    //This isn't accelerating properly trying again
    ships[shipIndex].coord.velX = fix16Add((cosFix16(test) / 9), ships[shipIndex].coord.velX);
    ships[shipIndex].coord.velY = fix16Add(-(sinFix16(test) / 9), ships[shipIndex].coord.velY);

    //Debugging
    /*char rad[16];
    uint16ToStr(test, rad, 4);
    VDP_drawText(rad, 12, 16);*/

    //make sure that the ship is not going too fast
    //We are going to use Pythagora's theorem to calculate total velocity... roughly
    //I'm not going to find square root of the total velocity because it will put too
    //much stress on the genesis and slow the game down.
    if (FIX16(10) < fix16Add(fix16Mul(ships[shipIndex].coord.velX,ships[shipIndex].coord.velX), fix16Mul(ships[shipIndex].coord.velY,ships[shipIndex].coord.velY)))
    {
        ships[shipIndex].coord.velX = vx;
        ships[shipIndex].coord.velY = vy;
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
    //Only fire bullets if the ship is alive
    if (ships[shipIndex].coord.isAlive == 1)
    {
        // If the ship fires a bullet and there is an open slot in the index queue
        // create a bullet at the ships position and make a sprite to represent the bullet
        if (bullets[ships[shipIndex].bulletIndex].coord.isAlive == 0)
        {
            //Get the position of the ship spawn bullet at position
            bullets[ships[shipIndex].bulletIndex].coord.x = fix16Add(ships[shipIndex].coord.x, FIX16(8));
            bullets[ships[shipIndex].bulletIndex].coord.y = fix16Add(ships[shipIndex].coord.y, FIX16(8));

            //Make the sprite
            bullets[ships[shipIndex].bulletIndex].coord.sprite =
                SPR_addSprite(&bullet, fix16ToInt(bullets[ships[shipIndex].bulletIndex].coord.x),
                                              fix16ToInt(bullets[ships[shipIndex].bulletIndex].coord.y),
                                               TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        }
        // else if the bullet queue is full take an existing bullet and its sprite
        // and set it to the firing position of the ship
        else
        {
            //Get the position of the ship spawn bullet at position
            bullets[ships[shipIndex].bulletIndex].coord.x = fix16Add(ships[shipIndex].coord.x, FIX16(8));
            bullets[ships[shipIndex].bulletIndex].coord.y = fix16Add(ships[shipIndex].coord.y, FIX16(8));
        }

        //Set the velocity based on rotation
        u16 test = convertToTable(ships[shipIndex].rotation);
        bullets[ships[shipIndex].bulletIndex].coord.velX = fix16Add((cosFix16(test) * FIX16(0.08)), ships[shipIndex].coord.velX);
        bullets[ships[shipIndex].bulletIndex].coord.velY = fix16Add(-(sinFix16(test) * FIX16(0.08)), ships[shipIndex].coord.velY);

        //Make sure the bullets don't go too fast.
        // Now that I think about it I'm not too sure if this
        // is necessary.
        if (bullets[ships[shipIndex].bulletIndex].coord.velX > FIX16(6))
        {
            bullets[ships[shipIndex].bulletIndex].coord.velX = FIX16(6);
        }
        if (bullets[ships[shipIndex].bulletIndex].coord.velY)
        {
            bullets[ships[shipIndex].bulletIndex].coord.velY = FIX16(6);
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

        bullets[ships[shipIndex].bulletIndex].bulletLimit = temp;

        /*
        //debugging show index
        char rad[16];
        uint16ToStr(bulletLimit[indexBullets - 32], rad, 3);
        VDP_drawText(rad, 14, 17);
        */

        //increment the index by 1
        ships[shipIndex].bulletIndex += 1;

        //Wrap the bullets only 14 bullets on screen
        if (ships[shipIndex].bulletIndex > playerAmmunition)
        {
            ships[shipIndex].bulletIndex = 0;
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
    u16 frame = fix16ToInt(ships[shipIndex].rotation) / 6;

    //Ship facing east
    if (ships[shipIndex].rotation >= FIX16(0) && ships[shipIndex].rotation <= FIX16(90) && ships[shipIndex].coord.isAlive == 1)
    {
        frame = 15 - frame;
        SPR_setHFlip(ships[shipIndex].coord.sprite, FALSE);
        SPR_setVFlip(ships[shipIndex].coord.sprite, FALSE);
    }
    else if (ships[shipIndex].rotation > FIX16(90) && ships[shipIndex].rotation <= FIX16(180) && ships[shipIndex].coord.isAlive == 1)
    {
        frame = frame - 15;
        SPR_setHFlip(ships[shipIndex].coord.sprite, TRUE);
        SPR_setVFlip(ships[shipIndex].coord.sprite, FALSE);
    }
    else if (ships[shipIndex].rotation > FIX16(180) && ships[shipIndex].rotation <= FIX16(270) && ships[shipIndex].coord.isAlive == 1)
    {
        frame = 15 - (frame - 30);
        SPR_setHFlip(ships[shipIndex].coord.sprite, TRUE);
        SPR_setVFlip(ships[shipIndex].coord.sprite, TRUE);
    }
    else if (ships[shipIndex].rotation > FIX16(270) && ships[shipIndex].rotation < FIX16(360) && ships[shipIndex].coord.isAlive == 1)
    {
        frame = (frame - 45);
        SPR_setHFlip(ships[shipIndex].coord.sprite, FALSE);
        SPR_setVFlip(ships[shipIndex].coord.sprite, TRUE);
    }

    /*
    //Debugging Show Index of Frame used
    char str2[16];
    uint16ToStr(frame, str2, 2);
    VDP_drawText(str2, 9, 12);
    */
    if (ships[shipIndex].coord.isAlive == 1)
    {
        SPR_setFrame(ships[shipIndex].coord.sprite, frame);
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

 /**************************************************************
 *                  !---Scree Wrap Elements---!
 * This function will wrap any entity on screen.
 *
 **************************************************************/

static void screenWrapElements() {
    fix16 * valX;
    fix16 * valY;
    u8 * living;

    // set the value of the x, y coordinates
    switch(byteParam1) {
    case rockType:
        valX = &rocks[byteParam2].x;
        valY = &rocks[byteParam2].y;
        living = &rocks[byteParam2].isAlive;
        break;
    case bulletType:
        valX = &bullets[byteParam2].coord.x;
        valY = &bullets[byteParam2].coord.y;
        living = &bullets[byteParam2].coord.isAlive;
        break;
    case shipType:
        valX = &ships[byteParam2].coord.x;
        valY = &ships[byteParam2].coord.y;
        living = &ships[byteParam2].coord.isAlive;
        break;
    }

    if(*living == 1)
    {
        //Screen rapping X+
        if (*valX > FIX16(302))
        {
            *valX = FIX16(-15);
        }
        //Screen Rapping X-
        if(*valX < FIX16(-15))
        {
            *valX = FIX16(302);
        }
        //Screen Rap Y+
        if (*valY > FIX16(206))
        {
            *valY = FIX16(-15);
        }
        //Screen Rap Y-
        if (*valY < FIX16(-15))
        {
            *valY = FIX16(206);
        }
    }
}

/********************************************************************************
*                   !---update Physics---!
* Procedure:
*   1. Advance bullets
*   2. Advance rocks
*   3. Advance ships
*   4. Screen wrap all entities
*   5. Handle the Collisions and increment score call other functions if needed
********************************************************************************/

static void updatePositions(){
    //index through the asteroids coordinates.
    u8 i = 0;
    u8 j;

    do
    {
        // 1. Move rocks and bullets and ships
        j = 0;

        //We are checking to see if the asteroid is dead because we don't want
        // to move dead entities
        byteParam2 = i;
        if (rocks[i].isAlive == 1)
        {
            rocks[i].x = fix16Add(rocks[i].x, rocks[i].velX);
            rocks[i].y = fix16Add(rocks[i].y, rocks[i].velY);
            byteParam1 = rockType;
            screenWrapElements();
            SPR_setPosition(rocks[i].sprite, fix16ToInt(rocks[i].x), fix16ToInt(rocks[i].y));
        }
        if (bullets[i].coord.isAlive == 1)
        {
            bullets[i].coord.x = fix16Add(bullets[i].coord.x, bullets[i].coord.velX);
            bullets[i].coord.y = fix16Add(bullets[i].coord.y, bullets[i].coord.velY);
            byteParam1 = bulletType;
            screenWrapElements();
            SPR_setPosition(bullets[i].coord.sprite, fix16ToInt(bullets[i].coord.x), fix16ToInt(bullets[i].coord.y));
        }
        if (i < numPlayers)
        {
            if (ships[i].coord.isAlive == 1) {
                ships[i].coord.x = fix16Add(ships[i].coord.x, ships[i].coord.velX);
                ships[i].coord.y = fix16Add(ships[i].coord.y, ships[i].coord.velY);
                byteParam1 = shipType;
                screenWrapElements();
                SPR_setPosition(ships[i].coord.sprite, fix16ToInt(ships[i].coord.x), fix16ToInt(ships[i].coord.y));
            }
        }

        //Check to see if the bullets need to die
        //due to how long they've been on screen
        //not working
        if (bullets[i].coord.isAlive == 1)
        {
            if(bullets[i].bulletLimit == counter)
            {
                SPR_releaseSprite(bullets[i].coord.sprite);
                bullets[i].coord.isAlive = 0;
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
        //only check for collisions with asteroids.
        //j is bullets
        j = 0;
        do {
            if(bullets[j].coord.isAlive == 1)
            {
                if (checkCollision(i, rockType, j, bulletType) == 1)
                {
                    //Add to the score when certain asteroids are hit
                    if(rocks[i].hitBox == 16)
                    {
                        Score += 20;
                    }
                    else if(rocks[i].hitBox == 8)
                    {
                        Score += 50;
                    }
                    else if(rocks[i].hitBox == 4)
                    {
                        Score += 100;
                    }
                    SPR_releaseSprite(rocks[i].sprite);
                    SPR_releaseSprite(bullets[j].coord.sprite);
                    createAsteroids(i);
                    //play asteroid destruction animation
                    explosion();
                    rocks[i].isAlive = 0;
                    bullets[j].coord.isAlive = 0;
                    numAsteroids--;
                    displayScore();
                }
            }
            j++;
        } while(j < playerAmmunition);

        // increment through all entities
        i++;
    } while (i < 32);
}

/***************************************************************
*                   !--Check Collision--!
*   This function is void. It will check if the sprites have
* have collided and it will release the sprites from memory
* will call the create asteroid function. Function will return
* a 1 when the asteroids touch for now.
*
* Version 2.0.0 I should maybe change this so it passes two 16-
* bit pointers which will then pass via 32 bit bus saturation.
* Might be a bit faster than passing 4 8-bit values.
***************************************************************/

/********
(u8 i, u8 j)
if (abs(fix16Sub(fix16Add(positions[j][0], intToFix16(hitBox[j])), fix16Add(positions[i][0], intToFix16(hitBox[i])))) < FIX16(hitBox[i]) &&
        abs(fix16Sub(fix16Add(positions[j][1], intToFix16(hitBox[j])), fix16Add(positions[i][1], intToFix16(hitBox[i])))) < FIX16(hitBox[i]))

********/

u8 checkCollision(u8 index1, u8 objectType1, u8 index2, u8 objectType2)
{
    Entity * object1;
    Entity * object2;

    //determine the first object
    switch(objectType1)
    {
        case rockType:
        object1 = &rocks[index1];
        break;
        case bulletType:
        object1 = &bullets[index1].coord;
        break;
        case shipType:
        object1 = &ships[index1].coord;
        break;
    }
    //determine the second object
    switch(objectType2)
    {
        case rockType:
        object2 = &rocks[index2];
        break;
        case bulletType:
        object2 = &bullets[index2].coord;
        break;
        case shipType:
        object2 = &ships[index2].coord;
        break;
    }

    if (abs(fix16Sub(fix16Add(object2->x, intToFix16(object2->hitBox)), fix16Add(object1->x, intToFix16(object1->hitBox)))) < FIX16(object1->hitBox) &&
        abs(fix16Sub(fix16Add(object2->y, intToFix16(object2->hitBox)), fix16Add(object1->y, intToFix16(object1->hitBox)))) < FIX16(object1->hitBox))
        return 1;
    else
        return 0;
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
    if (ships[0].coord.isAlive == 0 && ships[0].timer == -1)
    {
        if (counter == 0)
        {
            ships[0].timer = 39;
        }
        else
        {
            ships[0].timer = (counter - 1);
        }
    }
    if (ships[1].coord.isAlive == 0 && cooperative == 1 && ships[1].timer == -1)
    {
        if (counter == 0)
        {
            ships[1].timer = 39;
        }
        else
        {
            ships[1].timer = (counter - 1);
        }
    }
    else if (ships[0].timer == counter && ships[0].lives != 0 && ships[0].coord.isAlive == 0)
    {
        ships[0].rotation = 0;
        ships[0].coord.x = FIX16(160);
        ships[0].coord.y = FIX16(112);
        ships[0].coord.velX = 0;
        ships[0].coord.velY = 0;
        ships[0].coord.sprite = SPR_addSprite(&ship_sprite, fix16ToInt(ships[0].coord.x), fix16ToInt(ships[0].coord.y), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        //Default ship resting
        shipAnimation(46);
        ships[0].coord.hitBox = 8;
        ships[0].lives--;
        ships[0].timer = -1;
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
