// JUMPONG V3
// Copyright James Higgs 2023

// TODO:
// 1. Player can speed up ball by holding fire (?)
// CPU "player" (AI)

// 5200 bIOS IRQ vectors:
// FC03   ; Immediate IRQ  ($200)
// FCB8   ; Immediate VBI  ($202)
// FCB2   ; Deferred VBI   ($204)
// FEA1   ; DLI            ($206)
// FD02   ; Keyboard IRQ   ($208)
// FCB2   ; keypad continue vector ($20A)

//#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
//#include <6502.h>
//#include <atari5200.h>
#include "5200.h"							// 5200 mem-mapped registers 

// Put custom font at $B800 in ROM
#pragma rodata-name (push, "JJFONT")
//#include "fonts/font.h"
#include "fonts/fantasia_font.h"
#pragma rodata-name (pop)

#define SCREEN_ADDR	0x1000			// Address of the screen data
#define DL_ADDR 0x1300					// Address of the display list

#define POKE(a,b)		*(unsigned char *)(a) = (b)
#define PEEK(a)			*(unsigned char *)(a)

// Bat top and bottom Y-limit (in doubled scanlines)
#define BAT_LIMIT_TOP			19
#define BAT_LIMIT_BOTTOM	92

// Display list (copy to $1300)
const char displayList[] = {
	0x70, 0x70,                // skip 16 scan lines
	0x46,             		// set up gr .mode 6 screen
	0x00, 0x10,           // address of screen memory ($1000)
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,
	0x41,
	0x00, 0x13,           // jump back to top of list ($1300)
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00                // pad to 48 bytes	
};

// Note: ANTIC mode 6 can only display chars 0-63, blocks of 64 above that are displayed in a COLOUR1, COLOUR2 etc
//       eg: char 77 is cahr 13, but using COLOUR1 instead of COLOUR0
const char bgData[] = {
	0,0,0,0,0,42,53,45,48,47,46,39,0,54,19,0,0,0,0,0,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0, 34,57, 0, 42,53,45, 0, 40,41,39, 0, 18,16,18,19, 0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	0,0,0, 55, 50,41,52,52,37, 46,0,53,51,41, 46,39, 0,0,0,0,
	0,0,0,0,0, 0,0,0, 35,35,22,21, 0,0,0, 0,0,0,0,0,
	0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13
};

// Character translate table
// ' Note: char 33 = 'A', char 16 = '0' (zero)
// ' Trans table:
// ' 0                         16        0
// ' 1        !                17        1
// ' 2        "                18        2
// ' 3        #                19        3
// ' 4        $                20        4
// ' 5        %                21        5
// ' 6        &                22        6
// ' 7        '                23        7
// ' 8        (                24        8
// ' 9        )                25        9
// ' 10        *               26        :
// ' 11        +               27        ;
// ' 12        ,               28        <
// ' 13        -               29        =
// ' 14        .               30        >
// ' 15        /               31        ?

// ' 32        @               48        P
// ' 33        A               49        Q
// ' 34        B               50        R
// ' 35        C               51        S
// ' 36        D               52        T
// ' 37        E               53        U
// ' 38        F               54        V
// ' 39        G               55        W
// ' 40        H               56        X
// ' 41        I               57        Y
// ' 42        J               58        Z
// ' 43        K               59        ???
// ' 44        L               60        ???
// ' 45        M               61        ???
// ' 46        N               62        ???
// ' 47        0               63        ???

// Scan-doubled sprite data (for bat)
const char batData[] = {
    0xFC, 0xFC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xFC, 0xFC,
   	0
};
#define BAT_HEIGHT 24

// Ball bounce DY table
// BTABLE:                                ; bounce DY table
const char bouncDYTable[] = { 
	0x00, 0xFE, 0xFC, 0xFA, 0xF8, 0xF6, 0xF4, 0xF2,
	0x10, 0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02,
	0,0,0,0
};

// Sound envelopes
// Note: The 2 values per line are sent to AUDC1 and AUDF1 every frame

// PINGENV:                        ; ping sound envelope
const unsigned char pingEnv[] = {
   0xAA, 0xAF,										// AUDC1 0xAA = "pure tone" (hi nibble), volume = 0x0A (low nibble). AUDF1 = 0xAF (freq timer counter, higher vals = lower freq) 
   0xAF, 0xAF,
   0xA8, 0xAF,
   0xA4, 0xAF,
   0xA2, 0xAF,
   0xA1, 0xAF,
   0x00, 0x00
};

// NEWBALLENV:                     ; new ball sound
const unsigned char newBallEnv[] = {
   0xAA, 0x23,
   0xA0, 0x33,
   0xAA, 0x23,
   0xA0, 0x33,
   0xAA, 0x23,
   0x00, 0x00
};

const unsigned char speedUpEnv[] = {
   0xAA, 0x40,
   0xA0, 0x40,
   0xAA, 0x30,
   0xA0, 0x30,
   0xAA, 0x20,
   0x00, 0x00
};

// MISSENV:                        ; ball miss vol envelope
const unsigned char missEnv[] = {
   0xAA, 0x44,
   0xAB, 0x33,
   0xAC, 0x22,
   0xAD, 0x11,
   0xAE, 0x66,
   0xAD, 0x55,
   0xAC, 0x44,
   0xAB, 0x33,
   0xAA, 0x88,
   0xA9, 0x77,
   0xA8, 0x66,
   0xA8, 0x55,
   0xA7, 0xAA,
   0xA7, 0x99,
   0xA6, 0x88,
   0xA6, 0x77,
   0xA5, 0xCC,
   0xA5, 0xBB,
   0xA4, 0xAA,
   0xA4, 0x99,
   0xA3, 0xEE,
   0xA3, 0xDD,
   0xA2, 0xCC,
   0x00, 0x00
};

// WIN ENVELOPE
const unsigned char winEnv[] = {
	0xAA, 0x79,
	0xAA, 0x79,
	0xAA, 0x79,
	0xAA, 0x79,
	0xAA, 0x60,
	0xAA, 0x60,
	0xAA, 0x60,
	0xAA, 0x60,
	0xAA, 0x51,
	0xAA, 0x51,
	0xAA, 0x51,
	0xAA, 0x51,
	0xAA, 0x3C,
	0xAA, 0x3C,
	0xAA, 0x3C,
	0xAA, 0x3C,
	0xAA, 0x2F,
	0xAA, 0x2F,
	0xAA, 0x2F,
	0xAA, 0x2F,
	0xAA, 0x28,
	0xAA, 0x28,
	0xAA, 0x28,
	0xAA, 0x28,
	0xAA, 0x1D,
	0xAA, 0x1D,
	0xAA, 0x1D,
	0xAA, 0x1D,
	0xAA, 0x28,
	0xAA, 0x28,
	0xAA, 0x28,
	0xAA, 0x28,
	0xAA, 0x2F,
	0xAA, 0x2F,
	0xAA, 0x2F,
	0xAA, 0x2F,
	0xAA, 0x3C,
	0xAA, 0x3C,
	0xAA, 0x3C,
	0xAA, 0x3C,
	0xAA, 0x51,
	0xAA, 0x51,
	0xAA, 0x51,
	0xAA, 0x51,
	0xAA, 0x60,
	0xAA, 0x60,
	0xAA, 0x60,
	0xAA, 0x60,
	0xAA, 0x79,
	0xAA, 0x79,
	0xAA, 0x79,
	0xAA, 0x79,
	0x00, 0x00,
};

/*
PITCH VALUES FOR THE MUSICAL NOTES - AUDCTL=0, AUDC=$AX
AUDF Pitch Values for Notes
Note			AUDF Hex	AUDF Dec
C					1D				29					(high C)
B					1F				31
A# or Bb	21				33
A					23				35
G# or Ab	25				37
G					28				40
F# or Gb	2A				42
F					2D				45
E					2F				47
D# or Eb	32				50
D					35				53
C# or Db	39				57
C					3C				60
B					40				64
A# or Bb	44				68
A					48				72
G# or Ab	4C				76
G					51				81
F# or Gb	55				85
F					5B				91
E					60				96
D# or Eb	66				102
D					6C				108
C# or Db	72				114
Middle C	79				121						(middle C)
B					80				128
A# or Bb	88				136
A					90				144
G# or Ab	99				153
G					A2				162
F# or Gb	AD				173
F					B6				182
E					C1				193
D# or Eb	CC				204
D					D9				217
C# or Db	E6				230
C					F3				243						(low C)
*/


// Variables
unsigned char p1y;
unsigned char p2y;
char p1dy;
char p2dy;
unsigned char ballX;
int ballY;											// replaces BALLYLO and BALLYHI
char ballDX;
int ballDY;											// replaces BALLDYLO and BALLDYHI

// PL1 and PL2 scores
unsigned char scoreP1;
unsigned char scoreP2;
char server = 0;								// Who is serving (0 or 1)
char controller = 0;						// Which type of controller (joystick [0] or paddle [1])
char player2Type = 0;						// Player 2 human [0] or CPU [1]
unsigned char ballSpeed;
unsigned char bounces;

// Audio: Current envelope pointer (pointer into current envelope pos)
unsigned char *envelopePtr = 0;

// Set up graphics
void InitGFX()
{
	//SET DLIST=$BF00 MYDLIST
	// Copy dl data to RAM
	memcpy((void *)DL_ADDR, displayList, 48);
	// Copy bgdata to SCREEN RAM
	memcpy((void *)SCREEN_ADDR, bgData, 540);

	// Set displayist pointer (and shadow)
	// Set shadow vars first. VBI will set them back to these
	POKE(sDLISTL, 0x00);
	POKE(sDLISTH, 0x13);	
	POKE(DLISTL, 0x00);													// TODO - macro to do HIBYTE and LOBYTE of word
	POKE(DLISTH, 0x13);

	// Set character set pointer (must be on 1k boundary)
	POKE (CHBASE, 0xB8);						// custom charset (font.h) now at $B800 (see atari5200_with_font.cfg)

	// Initialise sprites
	POKE(GRACTL, 0x03);
	POKE(PMBASE, 0x20);							// $2000 (hi byte)  - must be on 1k boundary (or 512byte boundary?)
	// NB: PL0 data starts at PMBASE + 512 ! (missile data starts at PMBASE + 384)   (double-height sprite mode)
	// NB: might have to write to DMACTL here too
	//ANTIC.dmactl = 0x2A;		// %00101010 (Enabl DMA + enable plyer DMA + ENable standard playfield)
	// Want normal playfield width ($02) + M DMA ($04) + P DMA ($08) + PM haf-rez ($10 = 0) + DL DMA ($20)
	POKE(sDMACTL, 0x2E);  // normal playfield, 2-scanline sprites (double-height sprites)

	// Set playfield colours
	POKE(sCOLOR0, 0x1D);				// Color 0 shadow
	POKE(sCOLOR1, 0xFE);				// Color 1 shadow
	POKE(sCOLOR2, 0xCE);				// Color 2 shadow
	POKE(sCOLOR3, 0xBE);				// Color 3 shadow

	// Setup background colour
	POKE(sCOLBK, 0x02);

	// Set up players colours
	POKE(sCOLPM0, 0x4A);
	POKE(sCOLPM1, 0xBA);
	// Set up ball colour - ball is missile 2 so it can have a different colour to the bats
	POKE(sCOLPM2, 0xFE);

	// Set player X positions
	POKE(HPOSP0, 49);
	POKE(HPOSP1, 201);
}

// Print a string at the given location on screen
void print(unsigned char x, unsigned char y, char *s)
{
	unsigned char count;
	char *p;
	unsigned int offset;
	count = 20;									// Max string length allowed
	p = (char *)SCREEN_ADDR;
	offset = x + (y * 20);
	p += x;
	p += (y * 20);
	p += offset;
	p += ((unsigned int)y * 20);	
	p = (char *)SCREEN_ADDR + x + (y * 20);
	while (*s > 0)
		{
		*p = (*s - 32);
		p++; s++; count--;
		if (0 == count)
			break;
		}
}

// Select a keypad (0 to 3)
void keypad(unsigned char i)
{
	// if (T < 0 or T > 3): ERROROUT("Invalid keypad")
	// PRINTOUT("LDA", "#$" + HEX2(4 + T) + COMMENT(2))
	// PRINTOUT("STA", "CONSOL")
	POKE(CONSOL, (i + 4));				
}

// Read a key from the currently selected keypad
char inkey()
{
	// PRINTOUT("LDA", "KEY" + COMMENT(3))
	// PRINTOUT("LDX", "#$FF")
	// PRINTOUT("STX", "KEY")
	char c;
	c = PEEK(KBCODE);
	POKE(KBCODE, 0xFF);				// when writing, this reg is known as STIMER

	return c;

// Notes:
// SKCTL bit 1 enables the keyboard scanning. Note that this also 
// enables/disables the POT scanning. 
}

void WaitVSyncWithColourBars()
{
	// Wait for vsync (with colourbars)
	unsigned char vcount;
	unsigned char rtcLO;
	unsigned char c;

	vcount = PEEK(VCOUNT);				// NB - VCOUNT = current scanline / 2
	// if (vcount > 117)					// redundant!
	// 	return;

	rtcLO = PEEK(RTC_LO);
	while (vcount < 117)			// 118		NB: Use < or we may "miss" VCOUNT = 117
		{
		c = (vcount + rtcLO);
		// c = (c & 0x0F) | 0x70;
		POKE(WSYNC, 0);								// WSYNC *before* BG colour set, otherwise we migth be changing colour half way thru the line
		POKE(COLBK, c);

		vcount = PEEK(VCOUNT);
		}

	// POKE(COLBK, 0xFF);					// debug
	// POKE(WSYNC, 0);
	POKE(COLBK, 0x00);
}

void WaitVSync()
{
	// unsigned char rtcLO;

	// // Wait for next frame
	// rtcLO = *(unsigned char *)RTC_LO;
	// while (rtcLO == *(unsigned char *)RTC_LO)
	// 	{
	// 	asm("nop");
	// 	asm("nop");
	// 	asm("nop");
	// 	asm("nop");
	// 	asm("nop");
	// 	}

	unsigned char vcount;

	vcount = PEEK(VCOUNT);
	// if (vcount > 118)					// Redundant!
	// 	return;

	while (vcount < 118)			// 118		NB: Use < or we may "miss" VCOUNT = 118
		{
		POKE(WSYNC, 0);						// Wait for next scanline
		vcount = PEEK(VCOUNT);
		}		
}

// Clear sprite data (remove sprites from screen)
void ClearSprites()
{
	// memset((void *)0x2180, 0, 128);					// Clear missiles 0-3 at offset 384
	// memset((void *)0x2200, 0, 128);					// clear sprite 0 at offset 512
	// memset((void *)0x2280, 0, 128);					// clear sprite 1 at offset 640
	memset((void *)0x2180, 0, 384);							// clear missiles and sprites 0 and 1
}

void DrawPL1Sprite()
{
memset((void *)0x2200, 0, 128);
memcpy((char *)(0x2200 + p1y), batData, BAT_HEIGHT);			// PL0 sprite data starts at PMBASE + 512
}

void DrawPL2Sprite()
{
memset((void *)0x2280, 0, 128);
memcpy((char *)(0x2280 + p2y), batData, BAT_HEIGHT);			// PL0 sprite data starts at PLMBASE + 640
}

// Draw the ball "missile"
void DrawBall()
{
	char *p;

	// Clear missile data
	memset((void *)0x2180, 0, 128);

	// Draw ball (missile 0)
	p = (char *)(0x2180 + (ballY >> 8));
	*p++ = 0xF0;
	*p++ = 0xF0;
	POKE(HPOSM2, ballX); 
}

// Clear the text in the play area
void ClearPlayArea()
{
	// Need to clear 24 x 20-character lines, starting at $1000 + 40
	memset((void *)(SCREEN_ADDR + 40), 0, 480);
}

// CPU control (player 2)
void MoveCPU()
{
	unsigned char d;
	// The CPU paddle AI follows this strategy:
	//
	// If the ball is moving away then do nothing
	// Otherwise, predict where the ball will meet the paddle’s edge of the court.
	// If we have a prediction then move up or down to meet it.
	//
	// The CPU "skill" level is tied to:
	// - A delay CPU reaction time (frames) after you (human player) have hit the ball
	// - The speed at which the CPU bat is allowed to move
	// - Random error factor in reaction time and speed
	// - How long the CPU is tied to it's up/down movement decision (no of frames)

	// if (ballDX < 0)			// " Result of comparison is always false"
	// 	return;
	if (ballDX & 0x80)		// ballDX < 0 (ball moving left)
		return;

	// React yet?
	if (ballX < 100)			// TODO - tie to skill level, also add a random factor
		return;

	// Move paddle towards ball
	d = ((ballY >> 8) - (p2y + 12));
	if (d < 128)
		{
		if (p2y < BAT_LIMIT_BOTTOM)
			p2y++;
		}
	else
		{
		if (p2y > BAT_LIMIT_TOP)
		p2y--;
		}

}

//  --------------------------------------------------------------
//  PLAYER CONTROL
//  --------------------------------------------------------------
// Move the players based on joystick control
void MovePlayers()
{
	unsigned char potY;

	if (0 == controller)
		{
		// --- joystick controller

		// Move player 1
		p1dy = 0;
		potY = PEEK(sPOT1);					// JS 0 Y axis
		if (potY < 64)
			p1dy = -3;
		else if (potY > 192)
			p1dy = 3;
		p1y += p1dy;

		// Latch to top or bottom of bounds
		if (p1y < BAT_LIMIT_TOP)
			p1y = BAT_LIMIT_TOP;
		else if (p1y > BAT_LIMIT_BOTTOM)
			p1y = BAT_LIMIT_BOTTOM;

		// Move player 2
		if (0 == player2Type)
			{
			p2dy = 0;
			potY = PEEK(sPOT3);					// JS 1 Y axis
			if (potY < 64)
				p2dy = -3;
			else if (potY > 192)
				p2dy = 3;
			p2y += p2dy;

			// Latch to top or bottom of bounds
			if (p2y < BAT_LIMIT_TOP)
				p2y = BAT_LIMIT_TOP;
			else if (p2y > BAT_LIMIT_BOTTOM)
				p2y = BAT_LIMIT_BOTTOM;
			}
		else
			{
			// Apply CPU AI for P2
			MoveCPU();	
			}
		}
	else
		{
		// --- paddle controller

		// Move player 1
		potY = PEEK(sPOT0);
		if (potY > 185)
			potY = 185;
		if (potY < 38)
		 potY = 38;
		p1y = (potY >> 1);

		// Move player 2
		if (0 == player2Type)
			{
			potY = PEEK(sPOT1);
			if (potY > 185)
				potY = 185;
			if (potY < 38)
			potY = 38;
			p2y = (potY >> 1);
			}
		else
			{
			// Apply CPU AI for P2
			MoveCPU();
			}
		}
}

// ---------------------------------------------------------------------
//  SCORE ROUTINES
// ---------------------------------------------------------------------
void DrawScore()
{
	POKE(SCREEN_ADDR + 1, scoreP1 + 16);
	POKE(SCREEN_ADDR + 18, scoreP2 + 16);
}


//---------------------------------------------------------------------
// SOUND ROUTINES
//---------------------------------------------------------------------

// Per-frame sound update
void UpdateSound()
{
	char a;

	if (envelopePtr)
		{
		a = PEEK(envelopePtr);
		POKE(AUDC1, a);
		envelopePtr++;
		a = PEEK(envelopePtr);
		POKE(AUDF1, a);
		envelopePtr++;

		if (0 == a)
			envelopePtr = 0;			// Stop playing
		}
}

// Calculate ball vertical deflection change here, based on where the ball hits the bat
void Deflect(unsigned char batY)
{
// *** calc vertical deflection change here, based on
// *** where ball hits bat

// TODO - CHECK!
// +ve deflection for ball on lower half of bat, -ve for ball on top half of bat
unsigned char batCentreY;
int d;

batCentreY = batY + (BAT_HEIGHT / 2);
d = ((ballY >> 8) - batCentreY);
ballDY += (d * 2);														
bounces++;
}

// Show the title screen + animation
void DoTitleScreen()
{
	char key;
	unsigned char debounce;

	// Switch off audio
	POKE(AUDC1, 0);				// vol = 0, no 'volume only' mode, no distortion

	// Select keypad 1 for input (CONSOL)
	//*(char *)CONSOL = 1;	// SHould be 0?
	keypad(0);
	POKE(SKCTL, 2);	// SKCTL bit 1 enables kb scanning (also disables pots scanning?)

	// Set bat positions and directions
	p1y = 50; p1dy = 1;
	p2y = 50; p2dy = -1;

	// Show which controller
	print(3, 9, "SELECT INPUT *\0");

	if (0 == controller)
		print(5, 11, "-JOYSTICK-\0");
	else
		print(5, 11, "--PADDLE--\0");

	print(4, 14, "SELECT P2 #\0");

	if (0 == player2Type)
		print(5, 16, "--HUMAN--\0");
	else
		print(5, 16, "---CPU---\0");

	print(2, 19, "THEN PRESS START\0");

	debounce = 0;
	while (1)
		{
		// Wait for next frame
		WaitVSyncWithColourBars();

		// ' Draw player 1 and 2 sprites
		DrawPL1Sprite();
		DrawPL2Sprite();

		// Animate PL1
		p1y += p1dy;
		if (BAT_LIMIT_TOP == p1y)
			p1dy = 1;
		else if (BAT_LIMIT_BOTTOM == p1y)
			p1dy = -1;

		// Animate PL2
		p2y += p2dy;
		if (BAT_LIMIT_TOP == p2y)
			p2dy = 1;
		else if (BAT_LIMIT_BOTTOM == p2y)
			p2dy = -1;

		// Detect controller type change
		// ' *** emu bug: KBCODE doesn't update unless a key is pressed !!! ***
		if (debounce)
			{
			debounce--;
			}
		else
			{
			debounce = 10;
			key = inkey() & 0x0F;
			if (0x07 == key)						// Star button (F5 in Jum52)
				{
				if (0 == controller)
					{
					controller = 1;
					print(5, 11, "--PADDLE--\0");
					}
				else
					{
					controller = 0;
					print(5, 11, "-JOYSTICK-\0");
					}
				}
			else if (0x03 == key)				// Hash button (F6 in Jum52)
				{
				if (0 == player2Type)
					{
					player2Type = 1;
					print(5, 16, "---CPU---\0");
					}
				else
					{
					player2Type = 0;
					print(5, 16, "--HUMAN--\0");
					}
				}
			else if (0x09 == key)				// Start buttin (F1 in Jum52)
				break;
			}
		}
}

// Show player win message, and wait a few seconds
void ShowWinMessage()
{
	unsigned char countDown;

	if (9 == scoreP1)
		{
		print(3, 4, "PLAYER 1 WINS!");
		}
	if (9 == scoreP2)
		{
		print(3, 4, "PLAYER 2 WINS!");
		}

	// Start "win"" sound
	envelopePtr = (unsigned char *)winEnv;

	for (countDown = 240; countDown; countDown--)
		{
		WaitVSync();
		ClearSprites();			// kill some cycles 
		UpdateSound();			// This needed to finish playing the "miss" sound
		}
}

// Wait for current server to serve
void WaitForServe()
{
	unsigned char timer;

	// Wait for player to press trigger to release ball
	timer = 150;			// almost 3 secs
	while (1)
		{
    if (0 == server)
			{
			if (0 == PEEK(TRIG0))
				break;
			}
    else	// server = 1
			{
			if (0 == player2Type)
				{
				if (0 == PEEK(TRIG1))
					break;
				}
			else	// playerType = 1
				{
				if (0 == timer)
					break;
				}
			}

		WaitVSync();
		ClearSprites();
		DrawPL1Sprite();
		DrawPL2Sprite();
		MovePlayers();		
		UpdateSound();			// This needed to finish playing the "miss" sound from previous point scored
		timer--;
		}

	// Start ball acoording to server
	ballSpeed = 1; bounces = 0;
	if (0 == server)
		ballDX = 1;
	else
		ballDX = -1;
	// Serve
	ballDY = 2;

	// Ball in play
	envelopePtr = (unsigned char *)newBallEnv;
}

// Run the main game loop
void DoGame()
{
	// TODO - Split out subparts into their own functions

	// --------------------------------------------------------------
	//  Main GAME LOOP
	// --------------------------------------------------------------
	// Reset server to player 1, scores to 0
	server = 0; scoreP1 = 0; scoreP2 = 0;

	// Player has pressed START to get here, so play a chime
	envelopePtr = (unsigned char *)newBallEnv;

	// Clear middle of screen of text
	ClearPlayArea();

	// Main game outer loop
	while (1)
		{
		// Reset ball to centre
		ballX = 128; ballY = (62 << 8);		//(124 << 8);

		// Draw scores
		DrawScore();

		// JH - This has changed - win message now shown AFTER we return from this function
		if (9 == scoreP1)
			return;
		if (9 == scoreP2)
			return;

		// Wait for current server to serve
		WaitForServe();

		// Inner game loop
		while (1)
			{
			// Clear collision regs
			POKE(HITCLR, 0);							// Clear collision regs
			WaitVSync();
			DrawPL1Sprite();
			DrawPL2Sprite();
			DrawBall();
			MovePlayers();		

			// Move ball horizontally and vertically
			ballX = ballX + ballDX;
			ballY = ballY + ballDY;
		
			// Check for bounce against top and bottom walls
			if ((ballY >> 8) < BAT_LIMIT_TOP || (ballY >> 8) > 114)
				{
				// Bounce
				ballDY = -ballDY;
				envelopePtr = (unsigned char *)pingEnv;				// TODO - Different ping sound for top/bottom
				}

			// Hit bat of PL1 or PL2?
			if (1 == PEEK(M2PL))
				{
				// Ball hit PL1 bat
				ballDX = ballSpeed;
				// Calc vertical deflection change based on where ball hits bat
				Deflect(p1y);
				envelopePtr = (unsigned char *)pingEnv;
				}
			else if (2 == PEEK(M2PL))
				{
				// Ball hit PL2 bat
				ballDX = -ballSpeed;
				Deflect(p2y);
				envelopePtr = (unsigned char *)pingEnv;
				}

			// Check if we need to speed the ball up
			if (ballSpeed < 5)
				{
				if (50 == bounces)
					{
					bounces = 0;
					ballSpeed++;
					// Start speedup sound
					envelopePtr = (unsigned char *)speedUpEnv;
					}	
				}

			// Update sound
			UpdateSound();

			// Check if ball off screen, and exit inner loop if so
			if (ballX < 30 || ballX > 220)
				break;

			}	// end inner game loop

		// Ball off screen - who scored?
		if (ballX < 30)
			{
			scoreP2++;
			server = 1;
			}
		else
			{
			scoreP1++;
			server = 0;
			}

		// Trigger miss sound
		envelopePtr = (unsigned char *)missEnv;
		}	// play next ball

}

// ============================================================================
int main (void)
{
//	// Disable interrupts
//	ANTIC.nmien = 0x00;
//	ANTIC.nmires = 0x00;

	// DIsable interrupts
//	SEI();								// "Set interrupt disable"
	
	// Init graphics
	InitGFX();

	// // Init variables
	// controller = 0;										// controller = joystick
	// player2Type = 0;									// P2 is human	

	while (1)
		{

		// Show title screen until START pressed
		DoTitleScreen();
		// Run a new game
		DoGame();
		// Show the win message for a few seconds
		ShowWinMessage();
		}

	return 0; //EXIT_SUCCESS;
}

