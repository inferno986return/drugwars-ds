DrugWars DS Readme 1.2
======================

This game is a remake of the classic DrugWars played on several platforms.  It is a rather simple concept and is even easier to play.  The game has now outgrown the emulators on the market.  It is playable on some of them with graphics glitches and no sound, but is best played on your favorite hardware boot method.

Changes in 1.52
===============
- Fixed random ATM noises on main screen.
- Added icon and description.
- Changed Seattle picture.
- Fixed ending screen quirks.
- Cursor remembers position when you go back in menus.

Controls
========
A button	Advance to next menu, confirm options
B button	Go back to previous menu
Up button	Controls the menu cursor
		Add 1 drug to buy or sell
		Add $20 to deposit or withdraw amount
Down button	Controls the menu cursor
		Subtract 1 drug to buy or sell
		Subtract $20 to deposit or withdraw amount
L button	Add 10 drugs to buy or sell
R button	Subtract 10 drugs from buy or sell
Select+Start	Exit to main menu

If you forget a control mid-game, they are listed at the bottom of each screen.

Game Play
=========
You start out with some money and a loan to pay off (these amounts differ based on the difficulty) and you have 30, 60, or 90 days (based on selection in options) to pay off the loan and make as much money as you can as a drug dealer.  The basic concept is easy: buy low, sell high.  Certain random events can happen that affect your health, inventory, or the price of drugs currently.  Do not underestimate the power of keeping money in the bank, but do be careful as the feds have been known to seize accounts.  When the game is over, your total score of money on hand as well as money in the bank will be displayed.

Credits
=======
DragonMinded has written the framebuffer library and the game from scratch, using only his memory as to how the game was played, and some statistics off of the DEA's page on current drug prices.

Compiling
=========
This should compile under devkitarm r17 and the latest libnds.  To get libfb, just download it from http://www.youngmx.com/ndsdev/ and put it in the same folder as libnds.  Compiled .nds and .ds.gba should be copied to the main directory via the batch file, but if you are under a non-windows environment, you can find it as arm9.nds and arm9.ds.gba under the arm9 directory.  The ndslib.h file was placed there for two functions that the mod library needed in order to compile under libnds.  Thank you to Deku (http://deku.gbadev.org/) for the mod functions.

Copyright (C) 2006 Shaun Taylor
Written 1/15/2006