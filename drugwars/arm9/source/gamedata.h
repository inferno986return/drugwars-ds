
// modes

#define MAINMENU 0
#define GAME 1
#define OPTIONS 2
#define SCORES 3
#define CREDITS 4

#define EASY 0
#define MEDIUM 1
#define HARD 2
#define IMPOSSIBLE 3
#define DEBUG 4

#define DAYS_30 30
#define DAYS_60 60
#define DAYS_90 90

#define ACTIONS 0
#define BUY 1
#define SELL 2
#define PRICES 3
#define INVENTORY 4
#define TRAVEL 5
#define LOANSHARK 6
#define GAMBLE 7
#define BUYSUB 8
#define SELLSUB 9
#define COAT 10
#define CHALLENGE 11
#define FIGHT 12
#define BANK 13
#define DEPOSIT 14
#define WITHDRAW 15
#define BALANCE 16

#define MAX_CITIES 9
#define MAX_DRUGS 9

#define COCAINE 0
#define ACID 4
#define ECSTASY 7
#define LUDES 8

// data

char* cities[] = { "Atlanta",
					"Boston",
					"Chicago",
					"Denver",
					"Las Vegas",
					"Los Angeles",
					"New Orleans",
					"New York",
					"San Diego",
					"Seattle" };
					
int actions[] = { 	0,
						0,
						1,
						0,
						0,   //2,  <-- insert this to make gamble enabled
						0,
						0,
						0,
						0,
						0 };
						
char* drugs[] = {  "Cocaine",
					"Crystal Meth",
					"Heroin",
					"Angel Dust",
					"Acid",
					"Weed",
					"Speed",
					"Ecstasy",
					"Ludes" };
						
// 1st is spread, 2nd is lowest number
						
uint32 spreads[] = { 	25000, 15000,
						8000,  9000,
						10000, 5000,
						15000, 2000,
						2500, 1000,						
						400, 300,
						100, 100,
						70, 30,
						40, 20 };

