#include <stdio.h>
#include <stdlib.h>
#include <nds.h>
#include <string.h>
#include <libfb\libcommon.h>
#include <nds\arm9\sound.h>
//#include "gba_nds_fat\gba_nds_fat.h"
#include "Sound9.h"
#include "ndsload.h"
#include "font_kartika_14.h"

//graphics

#include "splash_bin.h"
#include "menu_bin.h"
#include "bg_bin.h"
#include "sprites.h"
#include "gamedata.h"
#include "signs.h"
#include "dayfont.h"

//sound
#include "beep_raw.h"
#include "cashreg_raw.h"
#include "gunshot_raw.h"
#include "maktone_mod.h"
#include "gryzor_mod.h"

uint32 mode;
uint32 submode;
uint16 scrVal = 0;
uint16 repVal = 0;

uint32 cursor;
uint32 count;
uint32 bcount;
uint32 cursorMax;
uint32 curKeys;
uint32 oldKeys;
uint32 repKeys;
uint32 diff;
uint32 days;

int health;
int dhealth;
uint32 hit;
uint32 dhit;

uint32 bankaccount;
uint32 bankamount;

uint32 day;
uint32 eMod;
uint32 whichMod;
uint32 mod;
uint32 city;
uint32 cash;
uint32 loan;
uint32 curdrug;
uint32 curprice;
uint32 pockets;

uint32 amounts[MAX_DRUGS];
uint32 prices[MAX_DRUGS];
uint32 oldprices[MAX_DRUGS];
uint32 stats[90][MAX_DRUGS];
uint16 stack[100];

int stackPos = -1;

#define SND_MEM_POOL_SIZE	256*1024
u32 *sndMemPool = (u32*)0x3000000;
uint16 *bBuffer;
uint16 cVal;

int posArr[12] = { 0, 10, 50, 150, 250, 256, 256, 250, 150, 50, 10, 0 };
int tArr[16] = { 1, 2, 3, -1, 4, 5, 6, -2, 7, 8, 9, -10, -10, 0, -10, -3 };

// Stack stuff

void pushStack(uint16 x)
{
	stackPos++;
	stack[stackPos] = x;
}

uint16 popStack()
{
	uint16 x = 0;
	
	if(stackPos != -1)
	{
		x = stack[stackPos];
		stackPos--;
	}
	
	return x;	
}

void clearStack()
{
	stackPos = -1;
}	

// DB stuff

void db_init()
{
	videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG_0x6000000);
	vramSetBankB(VRAM_B_MAIN_BG_0x6020000);
	bg_init();
      
    //set the control registers and rot scale information on the extended rotation backgrounds
    //bg 2
    BG2_CR = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(2);
    BG2_XDX = 1 << 8; //256 == 1 << 8 ?
    BG2_XDY = 0;
    BG2_YDY = 1 << 8;
    BG2_YDX = 0;
    BG2_CX = 0;
    BG2_CY = 0;
   
    //bg 3
    BG3_CR = BG_BMP16_256x256 | BG_BMP_BASE(8) | BG_PRIORITY(3);
    BG3_XDX = 1 << 8;
    BG3_XDY = 0;
    BG3_YDY = 1 << 8;
    BG3_YDX = 0;
    BG3_CX = 0;
    BG3_CY = 0;
	
	bBuffer = (uint16 *)malloc(256*192);
	cVal = 12;
}

void db_setBG(uint16 *bg)
{
	dmaCopy(bg,(uint16 *)BG_BMP_RAM(8),256*256*2);
	//for(int i=0;i<256*256;i++)
	//	((uint16 *)BG_BMP_RAM(8))[i] = (1 << 15) | bg[i];
}

void db_setFG()
{
	cVal = 0;
}

void db_startFG()
{
	cVal = 5;
}

void db_display()
{
	dmaCopy(bBuffer,(uint16 *)BG_BMP_RAM(0),256*192*2);
	//for(int i=0;i<256*192;i++)			
	//	((uint16 *)BG_BMP_RAM(0))[i] = bBuffer[i];	
}

void db_clear()
{
	memset(bBuffer,0,256*192*2);
	//for(int i=0;i<256*192;i++)
	//	bBuffer[i] = 0;
}

void db_loop()
{
	if(cVal < 12)
	{	
		if(cVal == 5)
		{
			// copy now
			
			db_display();			
		}
		
		if(cVal == 11)
		{
			// clear buffer
			
			db_clear();
		}
		
		BG2_CX = posArr[cVal] << 8;	
		cVal++;
	}
}

void db_dispString(int x, int y, char *w)
{
	dispString(x, y, w, bBuffer, 0, 0, 0, 255, 191);
}

void db_dispSprite(int x, int y, uint16 *sp, int tc)
{
	dispCustomSprite(x, y, sp, tc, 0xFFFF, bBuffer);
}

// Normal code

uint32 trueRand() {
	return (((rand() >> 8) & 0xFF) + (((rand() >> 8 ) & 0xFF) << 8)) + (((((rand() >> 8) & 0xFF) + (((rand() >> 8 ) & 0xFF) << 8))) << 16);
}

void vBlank()
{
	scanKeys();
}

void scrollBack()
{
	if(mode == GAME) {
		if(scrVal < 128) {
			scrVal++;
			BG3_CY = scrVal << 7;
		}	
	}
}

void waitForInput()
{
	uint16 keysPressed = keysHeld();
	
	while(keysPressed == 0)
	{	
		keysPressed = keysHeld();
		swiWaitForVBlank();
		swiWaitForVBlank();
		swiWaitForVBlank();
		db_loop();
	}	
	
	while(keysPressed != 0)
	{	
		keysPressed = keysHeld();
		swiWaitForVBlank();
		swiWaitForVBlank();
		swiWaitForVBlank();
		db_loop();
	}	
}

void waitForAB()
{
	uint16 keysPressed = keysHeld();
		
	while((keysPressed & KEY_A) || (keysPressed & KEY_B))
	{	
		keysPressed = keysHeld();
		swiWaitForVBlank();
		swiWaitForVBlank();
		swiWaitForVBlank();
		db_loop();
	}
	
	while(!(keysPressed & KEY_A) && !(keysPressed & KEY_B))
	{	
		keysPressed = keysHeld();
		swiWaitForVBlank();
		swiWaitForVBlank();
		swiWaitForVBlank();
		db_loop();
	}
	
	while((keysPressed & KEY_A) || (keysPressed & KEY_B))
	{	
		keysPressed = keysHeld();
		swiWaitForVBlank();
		swiWaitForVBlank();
		swiWaitForVBlank();
		db_loop();
	}
}

void travel()
{
	uint16 pressedYet = 0;
	uint16 keysPressed = keysHeld();
	uint16 xLoc = 0;
	uint16 yLoc = 0;
	uint16 xL = 0;
	uint16 yL = 0;
	TransferSoundData sClang = { gunshot_raw, gunshot_raw_size, 22050, 50, 64, 1 };
	
	db_clear();
	BG3_CY = 0;
	bg_swapBuffers();
	bg_dispString(0,0, "Thank you for choosing Almost There airlines, now with over 90% reliability!  The captain would like to remind you to buckle up, and in the event of a crash, screaming is not appreciated.");
	bg_swapBuffers();
	
	switch(city)
	{
		case 0: //Atlanta
			db_setBG((uint16 *)mapd_bin);
			xLoc = 163;
			yLoc = 67;
			break;
		case 1: //Boston		
			db_setBG((uint16 *)mapc_bin);
			xLoc = 235;
			yLoc = 60;
			break;
		case 2: //Chicago		
			db_setBG((uint16 *)mapc_bin);
			xLoc = 128;
			yLoc = 91;
			break;
		case 3: //Denver		
			db_setBG((uint16 *)mapa_bin);
			xLoc = 151;
			yLoc = 119;
			break;
		case 4: //Las Vegas		
			db_setBG((uint16 *)mapa_bin);
			xLoc = 75;
			yLoc = 142;
			break;
		case 5: //Los Angeles		
			db_setBG((uint16 *)mapa_bin);
			xLoc = 51;
			yLoc = 161;
			break;
		case 6: //New Orleans		
			db_setBG((uint16 *)mapd_bin);
			xLoc = 125;
			yLoc = 109;
			break;
		case 7: //New York		
			db_setBG((uint16 *)mapc_bin);
			xLoc = 223;
			yLoc = 78;
			break;
		case 8: //San Diego
			db_setBG((uint16 *)mapb_bin);
			xLoc = 58;
			yLoc = 75;
			break;
		case 9: //Seattle
			db_setBG((uint16 *)mapa_bin);
			xLoc = 57;
			yLoc = 30;
			break;
	}	
	
	if(xLoc <= 128)
		xL = 190;
	else
		xL = 5;
	yL = 66;
	
	dispCustomSprite(xL, yL, daySprite, 31775, 0xFFFF, (uint16 *)BG_BMP_RAM(8));
	dispCustomSprite(xL + 9, yL + 28, numPointers[day/10], 31775, 0xFFFF, (uint16 *)BG_BMP_RAM(8));
	dispCustomSprite(xL + 30, yL + 28, numPointers[day%10], 31775, 0xFFFF, (uint16 *)BG_BMP_RAM(8));
	
	// make sure plane is gone
	
	while(cVal < 12)
	{
		keysPressed = keysHeld();
		swiWaitForVBlank();
		swiWaitForVBlank();
		swiWaitForVBlank();
		db_loop();
		
		if(keysPressed != 0)
		{
			if(pressedYet == 1)
				return;
		}
		else
			pressedYet = 1;
	}
	
	// delay loop
	
	for(uint li=0;li<7;li++)
	{
		keysPressed = keysHeld();
		swiWaitForVBlank();
		swiWaitForVBlank();
		swiWaitForVBlank();
		
		if(keysPressed != 0)
		{
			if(pressedYet == 1)
				return;
		}
		else
			pressedYet = 1;
			
		if(li == 5)
			playSound(&sClang);
	}
	
	dispCustomSprite(xLoc-7, yLoc-7, xSprite, 31775, 0xFFFF, (uint16 *)BG_BMP_RAM(8));
	
	// delay loop 2
	
	for(uint li=0;li<30;li++)
	{
		keysPressed = keysHeld();
		swiWaitForVBlank();
		swiWaitForVBlank();
		swiWaitForVBlank();
		
		if(keysPressed != 0)
		{
			if(pressedYet == 1)
				return;
		}
		else
			pressedYet = 1;
	}
}

void dispMenu()
{
	bg_dispString(25, 20 + 5, "Start Game");
	bg_dispString(25, 32 + 5, "Options");
	bg_dispString(25, 44 + 5, "Credits");
	//bg_dispString(25, 56 + 5, "Exit");
	
	bg_dispSprite(13, 20 + (12 * cursor) + 5 - 1, cursorSprite, 0);
	
	char inv[500];
	sprintf(inv, "\t%c %c to move cursor\n\t%c to select\n\t%c to go back", BUTTON_UP, BUTTON_DOWN, BUTTON_A, BUTTON_B );
	bg_dispString(5, 148, inv);
	
	cursorMax=2;  //3;
}

void dispOptions()
{
	bg_dispString(25, 20 + 5, "Difficulty:");
	bg_dispString(25, 32 + 5, "Days");
	bg_dispString(25, 44 + 5, "Music");
	
	switch(diff) {
		case EASY:
			bg_dispString(100, 20+5, "Easy");
			break;
		case MEDIUM:
			bg_dispString(100, 20+5, "Normal");
			break;
		case HARD:
			bg_dispString(100, 20+5, "Hard");
			break;
		case IMPOSSIBLE:
			bg_dispString(100, 20+5, "Impossible");
			break;
		case DEBUG:
			bg_dispString(100, 20+5, "Debug Mode");
			break;
	}
	
	switch(days) {
		case DAYS_30:
			bg_dispString(100, 32+5, "30");
			break;
		case DAYS_60:
			bg_dispString(100, 32+5, "60");
			break;
		case DAYS_90:
			bg_dispString(100, 32+5, "90");
			break;
	}
	
	switch(eMod) {
		case 0:
			bg_dispString(100, 44+5, "Off");
			break;
		case 1:
			bg_dispString(100, 44+5, "Gryzor");
			break;
		case 2:
			bg_dispString(100, 44+5, "Maktone");
			break;
		case 3:
			bg_dispString(100, 44+5, "Switch every 5 days");
			break;
		case 4:
			bg_dispString(100, 44+5, "Switch every 10 days");
			break;
	}
		
	bg_dispSprite(13, 20 + (12 * cursor) + 5 - 1, cursorSprite, 0);
	
	char str[500];
	sprintf(str, "\t%c %c to move cursor\n\t%c to toggle\n\t%c to go back", BUTTON_UP, BUTTON_DOWN, BUTTON_A, BUTTON_B );
	bg_dispString(5, 148, str);
	
	cursorMax=2;
}

void dispCredits()
{
	bg_dispString(5, 20, "DrugWars for Nintendo DS is written by DragonMinded.  Shoutout to dsboi, dovoto, and the rest of #dsdev for helping me get this far!  Thanks to WolfieTaka for the idea.  Thanks to Patater for the dual background help.\n\nLong live NDS homebrew.\n\nVersion 1.52");
	
	char str[500];
	sprintf(str, "\t%c %c to go back", BUTTON_A, BUTTON_B );
	bg_dispString(5, 172, str);
}

void dispScores()
{
	bg_dispString(5, 5, "Game over!");
	
	if( health <= 0 ) {
		//oops you died
		bg_dispString(5, 5 + 24, "You have died!  You lose!");
	}
	else {	
		if( loan == 0 ) {
			// paid off, you can get scores!
			char str[500];
			sprintf(str, "You made $%d in %d days!  Congratulations!", (cash + bankamount), days);
			bg_dispString(5, 5 + 24, str);	
		}
		else {
			bg_dispString(5, 5 + 24, "You never paid back the loan shark!  You lose!");
		}
	}
	
	char str[500];
	sprintf(str, "\t%c %c to return to main menu", BUTTON_A, BUTTON_B );
	bg_dispString(5, 172, str);
}

uint32 getSpace()
{
	uint32 z = pockets;
	
	for(uint32 k=0;k<MAX_DRUGS;k++) {
		z -= amounts[k];
	}
	
	return z;
}

void drawBankStuff()
{
	char inv[500];
	
	if(bankaccount == 1) {
		if(	submode == DEPOSIT)
		{			
			dispCustomSprite(20, 20, bank, 31775, 0xFFFF, (uint16 *)BG_BMP_RAM(0));
			dispCustomSprite(30, 30, ameriBank, 0, 0xFFFF, (uint16 *)BG_BMP_RAM(0));
			
			sprintf(inv, "Current balance: $%d\n\nDeposit: $%d", bankamount, bcount);
			dispString(0,25, inv, (uint16 *)BG_BMP_RAM(0), 1, 30, 30, 225, 161);
		}
		if(submode == WITHDRAW)
		{
			dispCustomSprite(20, 20, bank, 31775, 0xFFFF, (uint16 *)BG_BMP_RAM(0));
			dispCustomSprite(30, 30, ameriBank, 0, 0xFFFF, (uint16 *)BG_BMP_RAM(0));
			
			sprintf(inv, "Current balance: $%d\n\nWithdraw: $%d", bankamount, bcount);
			dispString(0,25, inv, (uint16 *)BG_BMP_RAM(0), 1, 30, 30, 225, 161);
		}
		if(submode == BALANCE)
		{
			dispCustomSprite(20, 20, bank, 31775, 0xFFFF, (uint16 *)BG_BMP_RAM(0));
			dispCustomSprite(30, 30, ameriBank, 0, 0xFFFF, (uint16 *)BG_BMP_RAM(0));
			
			sprintf(inv, "Current balance: $%d", bankamount);
			dispString(0,25, inv, (uint16 *)BG_BMP_RAM(0), 1, 30, 30, 225, 161);
		}
		if(submode == BANK)
		{
			dispCustomSprite(20, 20, bank, 31775, 0xFFFF, (uint16 *)BG_BMP_RAM(0));	
			dispCustomSprite(30, 30, ameriBank, 0, 0xFFFF, (uint16 *)BG_BMP_RAM(0));
			dispString(0, 25, "Please choose from the options below:\n\n1: Deposit\n2: Withdraw\n3: Check Balance", (uint16 *)BG_BMP_RAM(0), 1, 30, 30, 225, 161);
		}
	}	
}

uint32 getLow(int d)
{
	int zx = day-7;
	uint32 pr = 0xFFFFFFFF;
	
	if(zx < 0)
		zx = 0;

	for(uint16 zy=zx;zy<day;zy++)
	{
		if(stats[zy][d] < pr)
			pr = stats[zy][d];
	}
	
	return pr;
}

uint32 getHigh(int d)
{
	int zx = day-7;
	uint32 pr = 0;
	
	if(zx < 0)
		zx = 0;

	for(uint16 zy=zx;zy<day;zy++)
	{
		if(stats[zy][d] > pr)
			pr = stats[zy][d];	
	}
	
	return pr;
}

void drawDrugHeader(uint32 d)
{
	db_dispSprite(52, 19, postIt, 31775);
	
	switch(d)
	{
		case 0: //cocaine
			db_dispSprite(98, 24, cocaine, 31775);
			break;
		case 1: //meth
			db_dispSprite(72, 24, meth, 31775);
			break;
		case 2: //heroin
			db_dispSprite(98, 24, heroin, 31775);
			break;
		case 3: //Angel Dust
			db_dispSprite(78, 24, dust, 31775);
			break;
		case 4: //Acid
			db_dispSprite(95, 24, acid, 31775);			
			break;
		case 5: //Weed
			db_dispSprite(100, 24, weed, 31775);			
			break;
		case 6: //Speed
			db_dispSprite(97, 24, speed, 31775);			
			break;
		case 7: //Ecstasy
			db_dispSprite(90, 24, ecstasy, 31775);				
			break;
		case 8: //Ludes
			db_dispSprite(101, 24, ludes, 31775);				
			break;
	}
}

void drawBuy(uint32 d)
{
	char wee[200];
	
	drawDrugHeader(d);

	setColor(0x47DF);
	drawInverse();
	
	// code here
	
	int low = getLow(d);
	int high = getHigh(d);
	
	sprintf(wee, "Price: $%d\n\n\n\n\nWeekly Spread:\n$%d-$%d", prices[d], low, high);
	db_dispString(55, 65, wee);
	
	// end code
	
	db_startFG();
	
	setColor(0xFFFF);
	drawNormal();
}

void drawSell(uint32 d)
{
	char wee[200];
	
	drawDrugHeader(d);
	
	setColor(0x47DF);
	drawInverse();
	
	// code here	
	
	int low = getLow(d);
	int high = getHigh(d);
	
	sprintf(wee, "Price: $%d\n\nLast purchase price:\n$%d\n\nWeekly Spread:\n$%d-$%d", prices[d], oldprices[d], low, high);
	db_dispString(55, 65, wee);
	
	// end code
	
	db_startFG();
		
	setColor(0xFFFF);
	drawNormal();
}

void drawCityTop()
{	
	switch(city) {
		case 0:
			bg_dispSprite(86, 5, atlantaSpr, 31775);
			break;
		case 1:
			bg_dispSprite(86, 5, bostonSpr, 31775);
			break;
		case 2:
			bg_dispSprite(81, 5, chicagoSpr, 31775);
			break;
		case 3:
			bg_dispSprite(84, 5, denverSpr, 31775);
			break;
		case 4:
			bg_dispSprite(70, 5, las_vegasSpr, 31775);
			break;
		case 5:
			bg_dispSprite(62, 5, los_angelesSpr, 31775);
			break;
		case 6:
			bg_dispSprite(62, 5, new_orleansSpr, 31775);
			break;
		case 7:
			bg_dispSprite(75, 5, new_yorkSpr, 31775);
			break;
		case 8:
			bg_dispSprite(71, 5, san_diegoSpr, 31775);
			break;
		default:
			bg_dispSprite(86, 5, seattleSpr, 31775);
			break;
	}
}

void dispGame()
{
	char citydisp[100];
	char inv[500];
	
	citydisp[0] = 0;
	inv[0] = 0;
		
	switch( submode ) {
		case ACTIONS:
			//sprintf(citydisp, "Day %d", day );	
			//bg_dispString(5, 5, citydisp);
			
			bg_dispString(25, 25+20, "Buy Drugs");
			bg_dispString(25, 25+20 + 12, "Sell Drugs");
			bg_dispString(25, 25+20 + 24, "Prices & Inventory");
			bg_dispString(25, 25+20 + 36, "Travel");
			bg_dispString(25, 25+20 + 48, "Bank Account");
			
			drawCityTop();
			
			cursorMax = 4;
			
			if( actions[city] == 1 ) {
				bg_dispString(25, 25+20 + 60, "Loan Shark");
				cursorMax = 5;
			}
			if( actions[city] == 2 ) {
				bg_dispString(25, 25+20 + 60, "Gamble");
				cursorMax = 5;
			}			
			
			bg_dispSprite(13, 25+20 + (12 * cursor) - 1, cursorSprite, 0);
			
			sprintf(inv, "\t%c %c to move cursor\n\t%c to select\n\t%c to quickly travel", BUTTON_UP, BUTTON_DOWN, BUTTON_A, BUTTON_X );
			bg_dispString(5, 148, inv);
			
			break;
		case BUY:
			//bg_dispString(5, 5, "Buy Drugs");	
			
			for(uint16 i=0;i<MAX_DRUGS;i++) {
				bg_dispString(25, 25 + (i*12), drugs[i]);
			}
			
			bg_dispSprite(13, 25 + (12 * cursor) - 1, cursorSprite, 0);
			
			/*
			if( cursor <= 2 ) {
				for(uint16 i=0;i<5;i++) {
					bg_dispString(25, 25 + (i*12), drugs[i]);
				}			
				bg_dispSprite(13, 25 + (12 * cursor) - 1, cursorSprite, 0);
				cursorMax = 9;
				
				bg_dispChar(125, 75, SCROLL_DOWN);
			}
			if(cursor > 2 && cursor < MAX_DRUGS - 3) {
				int k = 0;
				for(uint16 i=cursor-2;i<cursor+3;i++) {
					bg_dispString(25, 25 + ((k)*12), drugs[i]);
					k++;
				}
				bg_dispSprite(13, 25 + (12 * 2) - 1, cursorSprite, 0);
				
				bg_dispChar(125, 25,  SCROLL_UP);
				bg_dispChar(125, 75,  SCROLL_DOWN);
			}
			if(cursor >= MAX_DRUGS - 3) {
				int m = 0;
				for(uint16 i=MAX_DRUGS - 5;i<MAX_DRUGS;i++) {
					bg_dispString(25, 25 + ((m)*12), drugs[i]);
					m++;
				}
				bg_dispSprite(13, 25 + (12 * (cursor - (MAX_DRUGS - 5))) - 1, cursorSprite, 0);
				
				bg_dispChar(125, 25, SCROLL_UP);
			}
			*/
			
			sprintf(inv, "\t%c %c to move cursor\n\t%c to select\n\t%c to go back", BUTTON_UP, BUTTON_DOWN, BUTTON_A, BUTTON_B );
			bg_dispString(5, 148, inv);
			
			cursorMax = MAX_DRUGS-1;
			break;
		case SELL:
			//bg_dispString(5, 5, "Sell Drugs");			
			
			for(uint16 i=0;i<MAX_DRUGS;i++) {
				if(amounts[i] > 0)
					setColor(RGB15(31,31,31));
				else
					setColor(RGB15(21,21,21));
				bg_dispString(25, 25 + (i*12), drugs[i]);
			}
			
			setColor(RGB15(31,31,31));
			
			bg_dispSprite(13, 25 + (12 * cursor) - 1, cursorSprite, 0);
			
			/*
			if( cursor <= 2 ) {
				for(uint16 i=0;i<5;i++) {
					bg_dispString(25, 25 + (i*12), drugs[i]);
				}			
				bg_dispSprite(13, 25 + (12 * cursor) - 1, cursorSprite, 0);
				cursorMax = 9;
				
				bg_dispChar(125, 75, SCROLL_DOWN);
			}
			if(cursor > 2 && cursor < MAX_DRUGS - 3) {
				int k = 0;
				for(uint16 i=cursor-2;i<cursor+3;i++) {
					bg_dispString(25, 25 + ((k)*12), drugs[i]);
					k++;
				}
				bg_dispSprite(13, 25 + (12 * 2) - 1, cursorSprite, 0);
				
				bg_dispChar(125, 25,  SCROLL_UP);
				bg_dispChar(125, 75,  SCROLL_DOWN);
			}
			if(cursor >= MAX_DRUGS - 3) {
				int m = 0;
				for(uint16 i=MAX_DRUGS - 5;i<MAX_DRUGS;i++) {
					bg_dispString(25, 25 + ((m)*12), drugs[i]);
					m++;
				}
				bg_dispSprite(13, 25 + (12 * (cursor - (MAX_DRUGS - 5))) - 1, cursorSprite, 0);
				
				bg_dispChar(125, 25, SCROLL_UP);
			}
			*/
			
			sprintf(inv, "\t%c %c to move cursor\n\t%c to select\n\t%c to go back", BUTTON_UP, BUTTON_DOWN, BUTTON_A, BUTTON_B );
			bg_dispString(5, 148, inv);
			
			cursorMax = MAX_DRUGS - 1;
			break;		
		case INVENTORY:	
			sprintf(inv, "Money: \t\t\t$%d\nOwed: \t\t\t$%d", cash, loan);
			bg_dispString(25, 25, inv);
			
			cursorMax = MAX_DRUGS - 4;
			
			for(uint32 n=cursor;n<cursor+4;n++) {
				bg_dispString(25, 25 + ((n+3-cursor) * 12), drugs[n]);
				
				sprintf(inv, "%d", amounts[n]);
				bg_dispString(125, 25 + ((n+3-cursor) * 12), inv);
			}
			
			if( cursor > 0 ) {
				bg_dispChar(175, 25 + 36, SCROLL_UP);
			}
			
			if( cursor < cursorMax) {				
				bg_dispChar(175, 25 + 72,  SCROLL_DOWN);
			}
			
			sprintf(inv, "Spaces: \t\t%d/%d\nHealth: \t\t\t%d%c", getSpace(), pockets, health, '%');
			bg_dispString(25, 25 + 96, inv);
			
			sprintf(inv, "\t%c %c to scroll\n\t%c to go back", BUTTON_UP, BUTTON_DOWN, BUTTON_B );
			bg_dispString(5, 160, inv);
			
			break;
		case TRAVEL:	
			bg_dispString(5, 5, "Select a Destination . . .");	
			if( cursor <= 3 ) {
				for(uint16 i=0;i<7;i++) {
					bg_dispString(25, 25 + (i*12), cities[i]);
				}			
				bg_dispSprite(13, 25 + (12 * cursor) - 1, cursorSprite, 0);
				cursorMax = 9;
				
				bg_dispChar(125, 99, SCROLL_DOWN);
			}
			if(cursor > 3 && cursor < MAX_CITIES - 3) {
				int k = 0;
				for(uint16 i=cursor-3;i<cursor+4;i++) {
					bg_dispString(25, 25 + ((k)*12), cities[i]);
					k++;
				}
				bg_dispSprite(13, 25 + (12 * 3) - 1, cursorSprite, 0);
				
				bg_dispChar(125, 25,  SCROLL_UP);
				bg_dispChar(125, 99,  SCROLL_DOWN);
			}
			if(cursor >= MAX_CITIES - 3) {
				int m = 0;
				for(uint16 i=MAX_CITIES - 6;i<MAX_CITIES+1;i++) {
					bg_dispString(25, 25 + ((m)*12), cities[i]);
					m++;
				}
				bg_dispSprite(13, 25 + (12 * (cursor - MAX_CITIES + 6)) - 1, cursorSprite, 0);
				
				bg_dispChar(125, 25, SCROLL_UP);
			}
			
			sprintf(inv, "\t%c %c to move cursor\n\t%c to select\n\t%c to go back", BUTTON_UP, BUTTON_DOWN, BUTTON_A, BUTTON_B );
			bg_dispString(5, 148, inv);
			
			break;
		case BUYSUB:			
			sprintf(inv, "You can afford %d.\nYou can hold %d.\n\nPurchase: %d", (cash/prices[curdrug]), getSpace(), count);
			cursorMax = (cash/prices[curdrug]);
			
			bg_dispString(25, 25, inv);
			
			if(cursorMax > getSpace()) {
				cursorMax = getSpace();
			}
			
			sprintf(inv, "\t%c %c %c %c to change amount\n\t%c to buy\n\t%c to go back", BUTTON_UP, BUTTON_DOWN, BUTTON_L, BUTTON_R, BUTTON_A, BUTTON_B );
			bg_dispString(5, 148, inv);
			
			break;
		case SELLSUB:			
			sprintf(inv, "You have %d.\n\nSell: %d", amounts[curdrug], count);
			cursorMax = amounts[curdrug];
			bg_dispString(25, 25, inv);
			
			sprintf(inv, "\t%c %c %c %c to change amount\n\t%c to sell\n\t%c to go back", BUTTON_UP, BUTTON_DOWN, BUTTON_L, BUTTON_R, BUTTON_A, BUTTON_B );
			bg_dispString(5, 148, inv);			
			break;		
		case LOANSHARK:	
			//bg_dispString(5, 5, "Visit the Loan Shark");	
			
			bg_setClipping(0,0,245,255);			
			
			if( loan == 0 ) {
				bg_dispString(25, 25, "Hey, quit botherin' me.");			
				sprintf(inv, "\n\t%c to leave", BUTTON_B );
				bg_dispString(5, 172, inv);
			}
			else {
				if( loan > cash ) {
					if( day < 10 ) {
						bg_dispString(25, 25, "Hey, I don't see all of my money.  Come back when it's all there, and don't waste my time.");
					}
					if( day > 10 && day < 25 ) {
						bg_dispString(25, 25, "I'm getting fucking impatient.  I'd better see my money soon.");
					}
					if( day >= 25 ) {
						bg_dispString(25, 25, "Where the fuck is my money, pal.");
					}
					sprintf(inv, "\n\t%c to run away", BUTTON_B );
				}
				else {
					bg_dispString(25, 25, "That's my money all right.  Hand it over.");			
					sprintf(inv, "\t%c to pay up\n\t%c to run away", BUTTON_A, BUTTON_B );
				}
				bg_dispString(5, 160, inv);
			}
			
			cursorMax = 0;
			break;
		case COAT:		
			bg_setClipping(0,0,245,255);			
			
			if( cash >= 350 ) {
				bg_dispString(5, 5, "A street vender offers you a trenchcoat with more room to carry things.  He wants $350 for it.  Do you want to purchase it?");			
				sprintf(inv, "\t%c to purchase\n\t%c to decline", BUTTON_A, BUTTON_B );
				bg_dispString(5, 160, inv);
			}
			else {
				bg_dispString(5, 5, "A street vender offers you a trenchcoat with more room to carry things.  He wants $350 for it; too bad you don't have enough.");			
				sprintf(inv, "\t%c to decline", BUTTON_B );
				bg_dispString(5, 172, inv);
			}
			
			cursorMax = 0;
			break;
		case CHALLENGE:
			sprintf(inv, "You are jumped by a rival dealer.");
			bg_dispString(5, 5, inv);
			sprintf(inv, "\t%c to fight back\n\t%c to run away", BUTTON_A, BUTTON_B);
			bg_dispString(5, 160, inv);	
			
			cursorMax = 0;
			break;
		case FIGHT:
			bg_setClipping(0,0,245,255);
			
			if(dhit == 0) {
				// haven't started!
				bg_dispString(5,5, "The dealer taunts you and dares you to take the first swing.");
			}
			else {
				sprintf(inv, "You hit him for %d health.\nHe hits you for %d health.", hit, dhit);
				bg_dispString(5, 5, inv);			
			}
			
			sprintf(inv, "Your health: \t%d\nDealer health: \t%d", health, dhealth);
			bg_dispString(5, 5+36, inv);
			
			sprintf(inv, "\t%c to hit\n\t%c to run away", BUTTON_A, BUTTON_B);
			bg_dispString(5, 160, inv);
			break;
		case GAMBLE:
			//do something
			submode = ACTIONS;
			db_setFG();
			
			break;
		case BANK:
			//bg_dispString(5,5, "Bank Visit");
			
			bg_setClipping(0,0,245,255);
			
			if( bankaccount == 0 ) {
				if(cash >= 5250) {
					//bg_dispString(25, 25, "You don't have an account open.  Opening a new account costs $250, with a minimum $5000 deposit.  Accounts earn 5% interest daily.");
					
					sprintf(inv, "\t%c to open an account\n\t%c to leave", BUTTON_A, BUTTON_B);
					bg_dispString(5, 160, inv);
					
					cursorMax = 0;
				}
				else {
					//bg_dispString(25, 25, "You don't have an account open, but you dont have enough money to open a new account.  Opening a new account costs $250, with a minimum $5000 deposit.  Come back when you have the money.");
					
					sprintf(inv, "\t%c to leave", BUTTON_B);
					bg_dispString(5, 172, inv);
					
					cursorMax = 0;
				}
			}
			else {
				//bg_dispString(25, 25, "Deposit\nWithdraw\nCheck Balance");
				
				//bg_dispSprite(13, 25 + (12 * cursor) - 1, cursorSprite, 0);
			
				sprintf(inv, "\t%c to leave", BUTTON_B );
				bg_dispString(5, 172, inv);
				
				cursorMax = 0;			
			}
			
			bg_dispSprite(65, 25, ATM, 31775);
			
			break;
		case DEPOSIT:
			//bg_dispString(5,5, "Deposit Cash");
			
			sprintf(inv, "You have $%d you can deposit.", cash);			
			bg_dispString(25, 148, inv);
			
			cursorMax = cash;
			
			bg_dispSprite(65, 25, ATM, 31775);
			
			sprintf(inv, "\t%c to leave", BUTTON_B );
			bg_dispString(5, 172, inv);
			break;
		case WITHDRAW:
			//bg_dispString(5,5, "Withdraw Cash");
			
			//sprintf(inv, "You have $%d you can withdraw.", bankamount);			
			//bg_dispString(25, 125, inv);
			
			cursorMax = bankamount;
			
			bg_dispSprite(65, 25, ATM, 31775);
			
			sprintf(inv, "\t%c to leave", BUTTON_B );
			bg_dispString(5, 172, inv);
			break;
		case BALANCE:
			//bg_dispString(5,5, "Check Balance");
			
			//sprintf(inv, "You have $%d in your account.", bankamount);			
			//bg_dispString(25, 25, inv);
			
			cursorMax = 0;
			
			bg_dispSprite(65, 25, ATM, 31775);
			
			sprintf(inv, "\t%c to leave", BUTTON_B );
			bg_dispString(5, 172, inv);
			break;
	}
}

void setCityGraphics()
{
	swiWaitForVBlank();
	
	switch(city) {
		case 0:
			db_setBG((u16 *)atlanta_bin);
			break;
		case 1:
			db_setBG((u16 *)boston_bin);
			break;
		case 2:
			db_setBG((u16 *)chicago_bin);
			break;
		case 3:
			db_setBG((u16 *)denver_bin);
			break;
		case 4:
			db_setBG((u16 *)las_vegas_bin);
			break;
		case 5:
			db_setBG((u16 *)los_angeles_bin);
			break;
		case 6:
			db_setBG((u16 *)new_orleans_bin);
			break;
		case 7:
			db_setBG((u16 *)new_york_bin);
			break;
		case 8:
			db_setBG((u16 *)san_diego_bin);
			break;
		case 9:
			db_setBG((u16 *)seattle_bin);
			break;
	}
	
	scrVal = 0;
	BG3_CY = scrVal << 6;
}

void dispEvent()
{
	uint32 zRand = 0;		
	
	swiWaitForVBlank();
	bg_swapBuffers();
	
	if( actions[city] == 1 ) {
		// loan shark, does he want to find you?
		if( day >= 25 && loan > 0 ) {
			// oh shit, youve been late on your loan!
			submode = LOANSHARK;
			db_dispSprite(0, 70, loanShark, 31775);					
			db_startFG();
		}
	}
	
	zRand = trueRand()%100;
	
	if( zRand >= 90 ) {  // 10% chance of coke bust	
		bg_dispString(5, 5, "The cops have made a huge coke bust and wiped out the supply.\n\nCocaine prices are outrageous!");
		char str[500];
		sprintf(str, "\t%c %c to continue", BUTTON_A, BUTTON_B);
		bg_dispString(5, 172, str);			
		
		prices[COCAINE] =  (trueRand() >> 2)%(spreads[uint16(COCAINE*2)]);
		prices[COCAINE] += 100000;
		
		swiWaitForVBlank();
		bg_swapBuffers();
		waitForAB();
		
		return;
	}
	
	zRand = trueRand()%100;
	
	if( zRand >= 90 ) {  // 10% chance of acid bottoming out
		bg_dispString(5, 5, "Cheap homemade acid has flooded the market.\n\nAcid is now rediculously cheap.");
		char str[500];
		sprintf(str, "\t%c %c to continue", BUTTON_A, BUTTON_B);
		bg_dispString(5, 172, str);	
		
		prices[ACID] =  (trueRand() >> 2)%500;
		prices[ACID] += 200;		
		
		swiWaitForVBlank();
		bg_swapBuffers();
		waitForAB();
		
		return;
	}
	
	zRand = trueRand()%100;
	
	if( zRand >= 90 ) {  // 10% chance of a rave
		bg_dispString(5, 5, "There is a rave happening and ravers are desperate to get their hands on ecstasy.\n\nPrices for ecstasy have doubled.");
		char str[500];
		sprintf(str, "\t%c %c to continue", BUTTON_A, BUTTON_B);
		bg_dispString(5, 172, str);	
		
		prices[ECSTASY] += spreads[ECSTASY*2];
		
		swiWaitForVBlank();
		bg_swapBuffers();
		waitForAB();
		
		return;
	}	
	
	zRand = trueRand()%100;
	
	if( zRand >= 90 ) {  // 10% chance of ludes bottoming out
		bg_dispString(5, 5, "A rival dealer had his supply jacked and stolen ludes are selling cheap.\n\nPrices for ludes have bottomed out.");
		char str[500];
		sprintf(str, "\t%c %c to continue", BUTTON_A, BUTTON_B);
		bg_dispString(5, 172, str);	
		
		prices[LUDES] =  trueRand()%10;
		prices[LUDES] += 2;
		
		swiWaitForVBlank();
		bg_swapBuffers();
		waitForAB();
		
		return;
	}	
	
	zRand = trueRand()%100;
	
	if( zRand >= 90 ) {  // 10% chance of a coat upgrade
		submode = COAT;
		return;
	}	
	
	zRand = trueRand()%100;
	
	if( zRand >= 93 ) {  // 7% chance of a mugging
		char str[500];
		
		uint32 kk=0;
		if(cash > 0) {
			kk = trueRand()%(cash >> 1);
			cash -= kk;
		}
		uint32 jj = trueRand()%10 + 10;
		health -= (int)jj;
		
		sprintf(str, "While leaving the airport, you are mugged.\n\nYou lose %d health and $%d.", jj, kk);
		bg_dispString(5, 5, str);
		sprintf(str, "\t%c %c to continue", BUTTON_A, BUTTON_B);
		bg_dispString(5, 172, str);	
		
		swiWaitForVBlank();
		bg_swapBuffers();
		waitForAB();
		
		return;
	}
	
	zRand = trueRand()%100;
	
	if( zRand >= 93 ) {  // 7% chance of a rival dealer fight
		hit = 0;
		dhit = 0;
		submode = CHALLENGE;
		return;
	}
	
	zRand = trueRand()%100;
	
	if( (zRand >= 95) && (bankaccount == 1) ) {  // 5% chance of a bank seizure
		char str[500];
		
		bg_dispString(5, 5, "The feds got a tip on your bank account and seized the money.  The account was also closed.");
		sprintf(str, "\t%c %c to continue", BUTTON_A, BUTTON_B);
		bg_dispString(5, 172, str);	
		
		bankaccount = 0;
		bankamount = 0;
		
		swiWaitForVBlank();
		bg_swapBuffers();
		waitForAB();
		
		return;
	}
}

void initCity() 
{			
	for(uint16 k=0;k<MAX_DRUGS;k++) {		
		prices[k] =  (trueRand() >> 2)%(spreads[uint16(k*2)]);
		prices[k] += spreads[(k*2) + 1];
	}
	
	health += 5;
	if(health > 100) {
		health = 100;
	}

	setCityGraphics();
	
	uint32 toAdd = 0;
	
	if( day > 1 ) {
		// interest
		
		if( loan > 0 ) {
			switch(diff) {
				case EASY:
					toAdd = (uint16)(loan * .10);
					loan += toAdd;
					break;
				case MEDIUM:
					toAdd = (uint16)(loan * .15);
					loan += toAdd;
					break;
				case HARD:
					toAdd = (uint16)(loan * .20);
					loan += toAdd;
					break;
				case IMPOSSIBLE:
					toAdd = (uint16)(loan * .25);
					loan += toAdd;
					break;
			}
		}
		
		if(bankamount > 0) {
			toAdd = (uint16)(bankamount * .05);
			bankamount += toAdd;
		}
	}
}

void handleAirport()
{
	if(submode == TRAVEL)
	{
		switch(cursor+1)
		{
			case 2:			
			case 4:
				// erase last point of interest
				for(uint16 yh=20;yh<=100;yh++)
				{
					for(uint16 xh=156;xh<236;xh++)
						((uint16 *)BG_BMP_RAM(0))[(yh*256) + xh] = 0;
				}
				
				break;
			case 3:
				// display loanshark poi
				dispCustomSprite(156, 20, poI, 31775, 0xFFFF, (uint16 *)BG_BMP_RAM(0));
				dispString(156+6, 20+45, "Loan Shark", (uint16 *)BG_BMP_RAM(0), 0, 0, 0, 255, 191);
		}
	}
}

void updateStats()
{
	for(int i=0;i<MAX_DRUGS;i++)
		stats[day-1][i] = prices[i];
}

void initGame()
{
	setColor(RGB15(31,31,31));	
	db_setBG((u16 *)menu_bin);	
	bg_setBG((u16 *)bg_bin);
	
	db_dispSprite(5, 155, logoSprite, 0);					
	db_display();
	db_clear();
	
	mode=MAINMENU;	
	cursor = 0;
	curKeys = keysHeld();
	oldKeys = curKeys;
	days = DAYS_30;
	diff = MEDIUM;
	eMod = 3;
}

int main()
{	
	touchPosition touch;
	TransferSoundData sBeep = { beep_raw, beep_raw_size, 11025, 75, 64, 1 };
	TransferSoundData sChing = { cashreg_raw, cashreg_raw_size, 22050, 127, 64, 1 };
	bool updatePOI = false;

	//------------------------
	// start initializing crap
	//------------------------
	
    powerON(POWER_ALL_2D); // turn on everything

    irqSet(IRQ_VBLANK, vBlank);
	irqEnable(IRQ_VBLANK);	
	
	db_init();
	bg_init(); // init bottom screen	
	setFont((uint16 **)font_kartika_14);
	lcdSwap();
	
	//FAT_InitFiles();
	
	SndInit9 (); // initialize sounds
	SndSetMemPool (sndMemPool, SND_MEM_POOL_SIZE); // gotta do this for some reason i dont know
	
	setGenericSound(11025, 127, 64, 1);
	
	//-------------
	//finished init
	//-------------	
	
	initGame();
	
	while(1)
	{			
		switch(mode) {
			case MAINMENU:
				dispMenu();
				break;
			case OPTIONS:
				dispOptions();
				break;
			case CREDITS:
				dispCredits();
				break;
			case GAME:
				dispGame();
				break;
			case SCORES:
				dispScores();
				break;
		}
		
		// keypresses!
		
		curKeys = keysHeld();
		
		if((curKeys & KEY_UP) && !(oldKeys & KEY_UP)) {
			if( cursor > 0 ) {				
				cursor--;
					
				updatePOI = true;
			}
			if( count < cursorMax ) {
				count++;				
			}
		}
		
		if((curKeys & KEY_DOWN) && !(oldKeys & KEY_DOWN)) {
			if( cursor < cursorMax ) {
				cursor++;
				
				updatePOI = true;				
			}
			if( count > 0 ) {
				count--;				
			}			
		}
		
		if((curKeys & KEY_L) && !(oldKeys & KEY_L)) {
			count+=10;
			if( count > cursorMax ) {
				count = cursorMax;
			}			
		}
		
		if((curKeys & KEY_R) && !(oldKeys & KEY_R)) {
			if( count > 10 ) {
				count-=10;
			}
			else {
				count = 0;
			}			
		}
		
		if((curKeys & KEY_A) && !(oldKeys & KEY_A)) {
			// select menu item
			
			switch(mode) {
				case MAINMENU:
					switch(cursor) {
						case 0:
							//start game!
							
							mode = GAME;
							day = 1;
							mod = 1;
							whichMod = 1;
							switch(eMod) {
								case 1:
								case 3:
								case 4:
									SndPlayMOD((void *)gryzor_mod);
									break;
								case 2:
									SndPlayMOD((void *)maktone_mod);
									break;
							}
							city = 2;
							health = 100;
							bankaccount = 0;
							bankamount = 0;
							
							for(int k=0;k<MAX_DRUGS;k++) {
								amounts[k] = 0;
							}
							
							pockets = 100;
							
							submode = ACTIONS;
							db_setFG();
							
							swiWaitForVBlank();
							bg_swapBuffers();
							
							switch( diff ) {
								case EASY:
									cash = 5000;
									loan = 4000;
									bg_dispString(5,5, "You are tired of your crappy job, so you decide to make some money selling drugs.  You had a grand saved up from your shitty job at the fast food joint, and borrowed 4 grand more from the loan shark to get started.");
									break;
								case MEDIUM:
									cash = 4000;
									loan = 4000;
									bg_dispString(5,5, "You start out in the streets of Chicago, looking to make some money selling drugs.  You don't have any cash so you borrow 4 grand from the loan shark to get started.");
									break;
								case HARD:
									cash = 3000;
									loan = 4000;
									bg_dispString(5,5, "You got into a bind and owe a grand to a neighbor.  You decide to sell drugs to pay back the debt, but the neighbor wants it now, so you borrow 4 grand from the loan shark, giving a grand to the neighbor.");
									break;
								case IMPOSSIBLE:
									cash = 1000;
									loan = 5000;
									bg_dispString(5,5, "Your debts have been piling up and you now owe 4 grand, and people are starting to ask for the money.  You borrow 5 grand from the loan shark, using 4 grand to pay off the debts, and go into business selling drugs to hopefully make back the cash.");
									break;
								case DEBUG:
									cash = 100000;
									loan = 1000;
									bg_dispString(5,5, "This is the debug menu, you cheater.  Max days = 5, Health = 5, Loan = $1,000 no interest.  Cash = $100,000");
									days = 5;
									health = 5;
									break;
							}			
							
							char str[500];
							sprintf(str, "\t%c %c to continue", BUTTON_A, BUTTON_B);
							bg_dispString(5, 172, str);			
							
							swiWaitForVBlank();
							bg_swapBuffers();
							waitForAB();
							
							// calculate prices here, also display graphic for city
							
							initCity();							
							updateStats();
							
							// end calculate				
							
							clearStack();							
							break;
						case 1:
							//options!
							mode = OPTIONS;
							cursor = 0;
							
							db_dispSprite(5, 155, optionsSprite, 0);							
							db_setFG();
							
							break;
						case 2:
							//credits!
							mode = CREDITS;
							cursor = 0;
							
							db_dispSprite(5, 155, creditsSprite, 0);							
							db_setFG();
							
							break;
					}
					break;
				case CREDITS:
					mode = MAINMENU;
					cursor = 0;
					
					db_dispSprite(5, 155, logoSprite, 0);					
					db_setFG();
					
					break;
				case SCORES:
					mode = MAINMENU;
					db_setBG((u16 *)menu_bin);				
					swiWaitForVBlank();
					bg_swapBuffers();
					cursor = 0;
					
					db_dispSprite(5, 155, logoSprite, 0);					
					db_setFG();
					
					break;
				case OPTIONS:
					// do toggle stuff here!
					
					switch(cursor) {
						case 0: // difficulty
							diff++;
							
							if(diff > IMPOSSIBLE) {
								diff = EASY;
							}
							
							break;
						case 1: // days
							if(days == DAYS_90) {
								days = DAYS_30;
								break;
							}
								
							if(days == DAYS_60) {
								days = DAYS_90;
							}
						
							if(days == DAYS_30) {
								days = DAYS_60;
							}
							break;
						case 2:
							if(eMod == 4){
								eMod=0;
							}
							else {
								eMod++;
							}
							break;
					}
					break;
				case GAME:
					// HERE IS WHERE WE HANDLE THE IMPORTANT SHIT!
					
					switch( submode ) {
						case ACTIONS: // menu for actions
							switch( cursor) {
								case 0:
									submode = BUY;
									pushStack(cursor);
									cursor = 0;
									break;
								case 1:
									submode = SELL;
									pushStack(cursor);
									cursor = 0;
									break;
								case 2:
									submode = INVENTORY;
									pushStack(cursor);
									cursor = 0;
									
									db_dispSprite(52, 19, postIt, 31775);		
									
									char inv[100];
									
									setColor(0x47DF);
									drawInverse();
									
									for(uint32 n=0;n<MAX_DRUGS;n++) {
										sprintf(inv, "$%d", prices[n]);
										db_dispString(55, 25 + (16*n), drugs[n]);
										db_dispString(150, 25 + (16*n), inv);
									}
									
									setColor(RGB15(31,31,31));
									drawNormal();
									
									db_startFG();
									
									break;
								case 3:
									submode = TRAVEL;
									pushStack(cursor);
									cursor = 0;
									
									db_dispSprite(0, 140, plane, 31775);					
									db_startFG();
										
									break;
								case 4:
									submode = BANK;
									pushStack(cursor);
									cursor = 0;		
									
									db_dispSprite(20, 20, bank, 31775);
									
									if( bankaccount == 0 ) {
										if(cash >= 5250) {
											dispString(0, 0, "You don't have an account open.  Opening a new account costs $250, with a minimum $5000 deposit.  Accounts earn 5% interest daily.", bBuffer, 1, 30, 30, 225, 161);
										}
										else {
											dispString(0, 0, "You don't have an account open, but you dont have enough money to open a new account.  Opening a new account costs $250, with a minimum $5000 deposit.  Come back when you have the money.", bBuffer, 1, 30, 30, 225, 161);											
										}
									}
									else {
										db_dispSprite(30, 30, ameriBank, 0);
										dispString(0, 25, "Please choose from the options below:\n\n1: Deposit\n2: Withdraw\n3: Check Balance", bBuffer, 1, 30, 30, 225, 161);
									}
									
									db_startFG();
									break;
								case 5:
									if( actions[city] == 1 ) {									
										db_dispSprite(0, 70, loanShark, 31775);					
										db_startFG();
										
										submode = LOANSHARK;
									}
									if( actions[city] == 2 )
										submode = GAMBLE;
									
									pushStack(cursor);
									cursor = 0;
									break;
							}	
							break;
						case TRAVEL:
							if( city == cursor ) {
								break;
							}
							city = cursor;
							day++;
							clearStack();
							cursor = 0;
							submode = ACTIONS;
							db_setFG();
							
							travel();
							
							if(eMod==3) {
								if(mod==5) {
									mod=1;
									if(whichMod==1) {
										whichMod=0;
										SndPlayMOD((void *)maktone_mod);
									}
									else {
										whichMod=1;
										SndPlayMOD((void *)gryzor_mod);
									}
								}
								else {
									mod++;
								}
							}
							
							if(eMod==4) {
								if(mod==10) {
									mod=1;
									if(whichMod==1) {
										whichMod=0;
										SndPlayMOD((void *)maktone_mod);
									}
									else {
										whichMod=1;
										SndPlayMOD((void *)gryzor_mod);
									}
								}
								else {
									mod++;
								}
							}								
							
							// calculate prices here, also display graphic for city
							
							initCity();
							
							// end calculate
							
							dispEvent();
							updateStats();						
							
							if( day > days ) {
								SndStopMOD();
								
								db_dispSprite(5, 155, creditsSprite, 0);					
								db_setFG();
								
								mode = SCORES;
							}
							if( health <= 0 ) {
								SndStopMOD();
								
								db_dispSprite(5, 155, creditsSprite, 0);					
								db_setFG();
								
								mode = SCORES;
							}
							break;
						case LOANSHARK:
							if( loan > cash ) {
								// cant pay back now!
							}
							else {
								cash -= loan;
								clearStack();
								loan = 0;
								submode = ACTIONS;
								playSound(&sChing);
								db_setFG();
								cursor = 0;
							}
							break;
						case BUY:
							pushStack(cursor);
							curdrug = cursor;
							curprice = 0;
							count = 0;
							
							submode = BUYSUB;
							drawBuy(curdrug);
							break;
						case SELL:
							if(amounts[cursor] > 0)
							{
								pushStack(cursor);
								curdrug = cursor;
								curprice = 0;
								count = 0;
								submode = SELLSUB;
								
								drawSell(curdrug);								
							}
							break;
						case BUYSUB:
							amounts[curdrug] += count;
							cash -= count * prices[curdrug];
							
							oldprices[curdrug] = prices[curdrug];
							
							playSound(&sChing);
							
							clearStack();
							cursor = 0;
							submode = ACTIONS;
							db_setFG();
							break;
						case SELLSUB:
							amounts[curdrug] -= count;
							cash += count * prices[curdrug];
							
							playSound(&sChing);
							
							clearStack();
							cursor = 0;
							submode = ACTIONS;
							db_setFG();
							break;
						case COAT:
							if( cash >= 350 ) {
								cash -= 350;
								pockets += 100;
							}
							
							clearStack();
							cursor = 0;
							submode = ACTIONS;
							db_setFG();
							break;
						case CHALLENGE:
							clearStack();
							cursor = 0;
							dhealth = (int) (trueRand()%20 + 80);
							submode = FIGHT;
							break;
						case FIGHT:
							clearStack();
							hit = trueRand()%15 + 1;
							dhit = trueRand()%15 + 1;
							
							dhealth -= (int)dhit;
							
							if(dhealth <= 0) {
								// win!
								char str[500];
								uint16 jj = trueRand()%3000 + 1000;
								uint16 kk = trueRand()%10 + 2;
								uint16 ll = trueRand()%MAX_DRUGS;
								
								cash += jj;
								amounts[ll] += kk;
								
								swiWaitForVBlank();
								bg_swapBuffers();
								
								sprintf(str, "You have killed the rival dealer!  You look through his pockets and get $%d and %d %s.", jj, kk, drugs[ll]);
								bg_dispString(5,5, str);
								
								sprintf(str, "\t%c %c to continue", BUTTON_A, BUTTON_B);
								bg_dispString(5, 172, str);			
								
								swiWaitForVBlank();
								bg_swapBuffers();
								waitForAB();
								
								submode = ACTIONS;
								db_setFG();
							}
							else {								
								health -= (int)hit;
							}
							
							if(health <= 0) {
								// lose!
								SndStopMOD();
								
								db_dispSprite(5, 155, creditsSprite, 0);					
								db_setFG();
								
								mode = SCORES;
							}
							break;
						case BANK:
							pushStack(cursor);
							if(bankaccount == 0 && cash >= 5250) {
								bankaccount = 1;
								cash -= 5250;
								bankamount += 5000;
								submode = BANK;		
								drawBankStuff();					
							}							
							
							break;						
					}
					break;
			}
		}
		
		if((curKeys & KEY_B) && !(oldKeys & KEY_B)) {
			// back button!
			
			switch(mode) {
				case CREDITS:
					mode = MAINMENU;
					cursor = 0;
					
					db_dispSprite(5, 155, logoSprite, 0);					
					db_setFG();
					
					break;
				case OPTIONS:
					mode = MAINMENU;
					cursor = 0;
					
					db_dispSprite(5, 155, logoSprite, 0);					
					db_setFG();
					
					break;
				case SCORES:				
					mode = MAINMENU;
					bg_setBG((u16 *)menu_bin);			
					swiWaitForVBlank();
					bg_swapBuffers();
					cursor = 0;
					
					db_dispSprite(5, 155, logoSprite, 0);					
					db_setFG();
					
					break;
				case GAME:
					if( submode != ACTIONS ) {						
						switch( submode ) {
							case BUYSUB:
								submode = BUY;
								db_setFG();
								break;
							case SELLSUB:
								submode = SELL;
								db_setFG();
								break;
							default:
								submode = ACTIONS;
								db_setFG();
						}
						cursor = popStack();
					}
					break;
			}
		}
				
		// Y Key
		if((curKeys & KEY_X) && !(oldKeys & KEY_X)) {
			if(mode==GAME) {
				switch( submode ) {
					case ACTIONS:
						pushStack(cursor);
						submode = TRAVEL;
						cursor=0;
						
						db_dispSprite(0, 140, plane, 31775);					
						db_startFG();
						
						break;
				}	
			}
		}
			
		// restart key
		if(((curKeys & KEY_START) && (curKeys & KEY_SELECT)) && !((oldKeys & KEY_START) && (oldKeys & KEY_SELECT))) {
			SndStopMOD();
			BG3_CY = 0;
			initGame();
		}
		
		touch=touchReadXY();
		if((keysDown() & KEY_TOUCH) && !(touch.x == 0 || touch.y == 0) && (submode == BANK || submode == DEPOSIT || submode == WITHDRAW || submode == BALANCE))
		{
			if((touch.px >= 65 && touch.px <= 256-65) && (touch.py >= 25 && touch.py <= 25+102))
			{
				//we are in bounds.
				
				uint16 tXc = 0;
				uint16 tYc = 3;
				
				if(touch.px <= 65+23)
					tXc = 0;
				
				if(touch.px >= 65+26 && touch.px <= 65+49)
					tXc = 1;
				
				if(touch.px >= 65+52 && touch.px <= 65+75)
					tXc = 2;
				
				if(touch.px >= 65+78)
					tXc = 3;
					
				if(touch.py <= 25+23)
					tYc = 0;
				
				if(touch.py >= 25+26 && touch.py <= 25+49)
					tYc = 1;
				
				if(touch.py >= 25+52 && touch.py <= 25+75)
					tYc = 2;
				
				if(touch.py >= 25+78)
					tYc = 3;
					
				if(tArr[(tYc * 4) + tXc] != -10)
				{
					// play beep sound
					playSound(&sBeep);
				}
				
				switch(tArr[(tYc * 4) + tXc])
				{
					case -1: // OK pressed
						if(bankaccount == 1)
						{
							if(submode == DEPOSIT)
							{
								cash -= bcount;
								bankamount += bcount;
								submode = BANK;
								bcount = 0;
							}
							
							if(submode == WITHDRAW)
							{
								cash += bcount;
								bankamount -= bcount;
								submode = BANK;
								bcount = 0;
							}
							
							if(submode == BALANCE)
								submode = BANK;
						}
							
						break;
					case -2: // clear pressed
						bcount = 0;
						break;
					case -3: // cancel pressed
						bcount = 0;
						submode = BANK;
						break;
					case -10:
						break;
					default:
						if(submode == BANK && bankaccount == 1) {
							bcount = 0;
							if(tArr[(tYc * 4) + tXc] == 1)
								submode = DEPOSIT;
							if(tArr[(tYc * 4) + tXc] == 2)
								submode = WITHDRAW;
							if(tArr[(tYc * 4) + tXc] == 3)
								submode = BALANCE;
						}
						else
						{
							uint32 tmpX = (bcount * 10) + tArr[(tYc * 4) + tXc];
							
							if(tmpX <= cursorMax)
								bcount = tmpX;
						}
						
						break;
				}
				
				if(tArr[(tYc * 4) + tXc] != -10)
					drawBankStuff();
			}
		}
		
		trueRand();
		swiWaitForVBlank();
		if(bg_needsUpdate())
			bg_swapBuffers();
		scrollBack();
		if(updatePOI)
		{
			handleAirport();
			updatePOI = false;
		}
		db_loop();
		
		// code for repeating keys for left button, right button, up and down dpad keys.
		
		if(curKeys == repKeys) {
			//repVal++;			
			//if(repVal > 1) {				
				oldKeys = curKeys & (0xFFFF - KEY_UP - KEY_DOWN - KEY_R - KEY_L - KEY_X - KEY_Y);
			//	repVal = 0;
			//}
			//else {
			//	oldKeys = curKeys;
			//}
		}
		else {
			repKeys = 0;
			if(oldKeys == curKeys) {
				repVal++;
			}
			else {
				repVal = 0;
			}
			
			if( repVal > 4 ) {
				//its been held for 10 counts
				repKeys = curKeys;
				oldKeys = curKeys & (0xFFFF - KEY_UP - KEY_DOWN - KEY_R - KEY_L - KEY_X - KEY_Y);
				repVal = 0;
			}
			else {				
				oldKeys = curKeys;
			}
		}		
	}
}
