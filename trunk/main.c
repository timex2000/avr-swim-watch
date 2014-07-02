/*

SportsWatch 1.0
	
First full draft of the swim watch on breadboard. Features include:
	
	- Currently all hardware is on a breadboard, and powered (and programmed) using a Raspberry Pi. 
	- Using 0.96" OLED display
	- There are 3 buttons which are used for navigation and running
	- An external 16MHz crystal is used with no divide by 8
	- A 24LC64 EEPROM is used to store one swim
	- The OLED display and the EEPROM memory are both I2C controlled
	
	- Using u8glib to control the OLED display
	- On startup, the device goes straight to main menu containing two options:
		- Show the previous swim session
		- Record a new session
	- A simple switch based Finite State Machine is used to navigate the system (including the menu, showing the previous swim, and recording the current one)
	- When showing the previous swim, each length can be shown - one at a time - by pressing the up or down buttons (return to the main menu using the select button)
	- The display shows the selected length's time in hh:mm:ss (need to remove the hh)
	- When going from main menu to record new swim, a new swim session is started and recorded to the EEPROM.
	- New lengths are recorded by pressing the select button (return to the main menu using the up or down buttons)
	- The previous length time and total time of the current swim are shown in hh:mm:ss format
	- Number of lengths, and length times are stored as uint8, whereas the total swim time is stored in uint16
	- Timer1 interrupt is set for every 0.1 sec, potentially giving 0.1 accuracy, though at the moment, the length times are stored with 1 sec resolution
	
-------------------------------------------------
	
	Next step:
		Clean up code:
			- Add enums for menu and current state
			- Combine some of the variables together into structs
			- Clean up some of the LCD printing routings (better use of char[] creation)
			- Sort through comments
			- Put into version 1.0 to put online (with version comments for version 0.* tidied up and made relivant
			
	
	---------------------------------------------


*/ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
	
#include "u8g.h"
#include "24c64.h"

volatile uint8_t lengths = 255;
volatile uint16_t lengthTime = 0;
volatile uint8_t ms100 = 0;					// Stores the amount of 100ms's in the current 1 second count
volatile uint16_t totalTime = 0;
volatile uint8_t currentState = 1;
volatile uint8_t extra10ms = 0;

void chooseNextState(uint8_t);

// Variables for menu (only main menu for now).
#define MAIN_MENU_ITEMS 2
char *mainMenuStrings[MAIN_MENU_ITEMS] = {"Last swim", "New Swim"};
u8g_t u8g;
volatile uint8_t menu_current = 0;
volatile uint8_t up = 0;
volatile uint8_t down = 0;
volatile uint8_t inMainMenu, inSwimMode, inLastView;

volatile uint8_t showLength = 1;
volatile uint8_t showLenths = 0;

uint8_t currentShowLength = 1;
uint8_t showLengthAmounts = 0;

uint8_t lastLengthTime = 0;

// ------------------------------------ Interrupt inits

// Initialise the Timer counters (only Timer1 used at the moment)
void timerInit(void){ 
	TCCR1A = (1 << WGM12);
	TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12);				// 64 prescaler
	TIMSK1 = (1 << OCIE1A);
	/* 	16MHz / 64 = 250,000MHZ
		= 0.000004 sec
		(target time = (timer resolution) * (# timer counts + 1)
		(# timer counts +1) = (target time) / (timer resolution)
		= (0.1 s) / (0.000004)				
		(# timer counts) = 25000 - 1 = 24999 */	
		// Gives 0.1 sec resolution
	OCR1A = 24999;
}

// Initialising the button press interrupts
// INT0 = select button, D6 = up, D7 = down
void initInt(void){
	EIMSK |= (1 << INT0);				// Enable INT0
	EICRA |= (1 << ISC01);				// Trigger on falling edge of INT0

	// Enabling pin change interrupt for D6 and D7 (up and down in the menu buttons)
	PCICR |= (1 << PCIE2);
	PCMSK2 |= (1 << PD6) | (1 << PD7);
}

// ------------------------------------ Other init functions

// Setting up the OLED display
void u8g_setup(void){
	u8g_InitI2C(&u8g, &u8g_dev_ssd1306_128x64_i2c, U8G_I2C_OPT_NONE);
}

// ------------------------------------ Interrupt call functions

// INT0 falling edge interrupt
// With delay based debounce
ISR(INT0_vect){
	_delay_ms(50);				// See if I can reduce this
	if (!(PIND & (1 << PD2))){
		// Choosing next state
		chooseNextState(3);
	}
	return;
}

// Button press interrupt
ISR(PCINT2_vect){
	// Only interested if the button has been pressed
	if (!(PIND & (1 << PD6)) || !(PIND & (1 << PD7))){	
		_delay_ms(50);  			// // See if I can reduce this delay

		if (!(PIND & (1 << PD6))){
			chooseNextState(1);
		}
		else if (!(PIND & (1 << PD7))){
			chooseNextState(2);
		}
	}
	return;
}

// Timer1 overflow
ISR (TIMER1_COMPA_vect){
	if (lengths < 255){
		// Check is required number of 50ms have pass
		// 10 x 10ms = 1s
		if (ms100++ == 9){    
			ms100 = 0;
			lengthTime++;
		}
	}
}

// ------------------------------------ OLED drawing functions

// Draws the main menu on the display (with the current selection being highlighted)
// Is there a way to optimise this?
void drawMainMenu(void){
	uint8_t i, h;
	u8g_uint_t w, d;
	u8g_SetFont(&u8g, u8g_font_6x13);
	u8g_SetFontRefHeightText(&u8g);
	u8g_SetFontPosTop(&u8g);
	h = u8g_GetFontAscent(&u8g) - u8g_GetFontDescent(&u8g);
	w = u8g_GetWidth(&u8g);

	// Doing the actual drawing
	u8g_FirstPage(&u8g);
	do{
		for (i = 0; i < MAIN_MENU_ITEMS; i++){
			d = (w - u8g_GetStrWidth(&u8g, mainMenuStrings[i])) / 2;
			u8g_SetDefaultForegroundColor(&u8g);
			if (i == menu_current){
				u8g_DrawBox(&u8g, 0, i*h+1, w, h);
				u8g_SetDefaultBackgroundColor(&u8g);
			}
			u8g_DrawStr(&u8g, d, i*h, mainMenuStrings[i]);
		}
	} while (u8g_NextPage(&u8g));
}

// Updates the menu if up or down button are called
void updateMainMenu(uint8_t up_down){
	// 1 = up, 0 = down
	if (up_down == 1){
		menu_current++;
		if (menu_current >= MAIN_MENU_ITEMS)
		menu_current = 0;
	}
	else if (up_down == 0){
		if (menu_current == 0)
		menu_current = MAIN_MENU_ITEMS;
		menu_current--;
	}
}

// ------------------------------------ Functions required for states

// Converts time into string to be printed in format hh:mm:ss
char* timeToString(uint16_t time, char *rS){
	//char rS[9];

	// Calculating the required values
	uint8_t seconds = time % 60;
	time /= 60;
	uint8_t minutes = time % 60;
	time /= 60;
	uint8_t hourss = time;
	
	// Converting the values into different strings
	char strHours[3], strMins[3], strSecs[3];
	//convertTime(lastLengthTime);
	itoa(hourss, strHours, 10);
	itoa(minutes, strMins, 10);
	itoa(seconds, strSecs, 10);
	
	// Combining the strings together into one.
	// Sure there must be a better way to do this!
	if (hourss < 1){
		rS[0] = '0';
		rS[1] = '0';
	}
	else if (hourss < 10){
		rS[0] = '0';
		rS[1] = strHours[0];
	}
	else{
		rS[0] = strHours[0];
		rS[1] = strHours[1];
	}
	rS[2] = ':';
	if (minutes < 1){
		rS[3] = rS[4] = '0';
	}
	else if (minutes < 10){
		rS[3] = '0';
		rS[4] = strMins[0];
	}
	else{
		rS[3] = strMins[0];
		rS[4] = strMins[1];
	}
	rS[5] = ':';
	if (seconds < 10){
		rS[6] = '0';
		rS[7] = strSecs[0];
	}
	else{
		rS[6] = strSecs[0];
		rS[7] = strSecs[1];
	}
	rS[8] = 0x00;
	
	return rS;
}

void showNextLength(void){
	if (currentShowLength++ >= showLengthAmounts)
	currentShowLength = 1;
}

void showPreviousLength(void){
	if (currentShowLength-- == 1)
	currentShowLength = showLengthAmounts;
}

void addLength(void){
	lengths++;
	EEWriteByte(2, lengths);
	EEWriteByte(2+lengths, lengthTime);	
	totalTime += lengthTime;
	// Adding the extra total time from the 10ms
	extra10ms += ms100;
	if (extra10ms >= 10){
		totalTime += 1;
		extra10ms -= 10;
	}
	lastLengthTime = lengthTime;
	lengthTime = 0;
}

// ------------------------------------ FSM State Functions

// First option of main menu
void mainMenuOne(void){
	drawMainMenu();  
}
		
// Second option of main menu
void mainMenuTwo(void){
	drawMainMenu();
}

// State for showing the current swim session
// Display shows the number of lengths, the last length time, and the total time (from the last length being recorded).
void recordSwim(void){
	
	char str1[4];
	// Not sure if itoa is the best way to do this
	itoa(lengths, str1, 10);
	char rS[9];
	
	u8g_FirstPage(&u8g);
	u8g_SetDefaultForegroundColor(&u8g);
	do{
		u8g_DrawStr(&u8g, 0, 5, "Lengths: ");
		u8g_DrawStr(&u8g, 90, 5, str1);
		
		u8g_DrawStr(&u8g, 0, 20, timeToString(lastLengthTime, rS));
		u8g_DrawStr(&u8g, 0, 35, timeToString(totalTime, rS));

	} while (u8g_NextPage(&u8g));
}

// State for showing the times for the last recorded swim
void showHistory(void){
	// Find current amount of lengths
	showLengthAmounts = EEReadByte(2);
	// Display the amount of lengths and the time taken for "currentShowLength"
	// Find the time for the correct length.
	uint8_t curTime = EEReadByte(2+currentShowLength);
	
	// Now do the display
	char str1[4], str3[4], rS[9];
	itoa(showLengthAmounts, str1, 10);
	itoa(currentShowLength, str3, 10);
	
	
	u8g_FirstPage(&u8g);
	u8g_SetDefaultForegroundColor(&u8g);
	do {
		u8g_DrawStr(&u8g, 0, 15, "Lengths: ");
		u8g_DrawStr(&u8g, 90, 15, str1);
		
		// Show the first length
		u8g_DrawStr(&u8g, 0, 30, "Length ");
		u8g_DrawStr(&u8g, 50, 30, str3);
		u8g_DrawStr(&u8g, 60, 30, timeToString(curTime, rS));
	} while (u8g_NextPage(&u8g));
}

// Choosing what to do based on current state and event
void chooseNextState(uint8_t event){
	/* currentState:
		1 = main menu 1
		2 = main menu 2 
		3 = show history
		4 = record swim
	*/
	switch(currentState){
	case 1:
		switch(event){
		case 1:					// Button 1
			updateMainMenu(1);
			mainMenuTwo();
			currentState = 2;
			break;
		case 2:					// Button 2
			updateMainMenu(0);
			mainMenuTwo();
			currentState = 2;
			break;
		case 3:					// Button 3
			currentShowLength = 1;	// Show the first length time when we first start showing
			showHistory();
			currentState = 3;
			break;
		default:
			mainMenuOne();
			currentState = 1;
		}
		break;
	case 2:
		switch(event){
		case 1:
			updateMainMenu(1);
			mainMenuOne();
			currentState = 1;
			break;
		case 2:
			updateMainMenu(0);
			mainMenuOne();
			currentState = 1;
			break;
		case 3:
			ms100 = 0;
			lengths = 0;
			lengthTime = 0;
			lastLengthTime = 0;
			totalTime = 0;					
			recordSwim();
			//currentShowLength = 1;
			//showHistory();
			currentState = 4;
			break;
		}
		break;
		// In show history state
	case 3:
		switch(event){
		case 1:
			showPreviousLength();
			showHistory();
			currentState = 3;
			break;
		case 2:					
			showNextLength();
			showHistory();
			currentState = 3;
			break;
		case 3:
			up = down = 0;
			menu_current = 0;
			mainMenuOne();
			currentState = 1;
			break;
		}
		break;
	case 4:
		switch(event){
		case 1:
			up = down = 0;
			menu_current = 1;
			mainMenuTwo();
			currentState = 2;
			break;
		case 2:
			up = down = 0;
			menu_current = 1;
			mainMenuTwo();
			currentState = 2;
			break;
		case 3:				
			addLength();
			ms100 = 0;
			recordSwim();
			currentState = 4;
			break;
		}
		break;
	}
}					

int main(void){
	// Inilialising port B as output for the LCD display
	DDRB = 0xFF;
	// Initliasing pin D.2 (INT0) as as input. This will be connected to button.
	//DDRD |= (1 << PD2);
	DDRD = 0xFF;
	// Pull up resistors for input
	//PORTD |= (1 << PD2);
	PORTD = 0xFF;
	// D6 = up
	// D7 = down

	// Initialising stuff
	initInt();
	timerInit();  
	u8g_setup();
	EEOpen();  

	sei();

	currentState = 1;
	chooseNextState(10);
	while(1){
	}

	return (0);
}
