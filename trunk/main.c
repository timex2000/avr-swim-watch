/*

SportsWatch 2.0
28-07-2014

*/

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
	
#include "u8g.h"
#include "24c64.h"

// Structs definitions
volatile struct buttonsState_t {
	uint8_t anyPressed;
	uint8_t upPressed;
	uint8_t downPressed;
	uint8_t selectPressed;
	uint8_t debounceMs;			// ms test for debouncing
	uint8_t timer0Ints;			// number of times timer0 interrupt has been called
} buttons;

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
volatile uint8_t stateChanged = 0;
volatile uint8_t updateScreen = 0;

// ------------------------------------ Interrupt inits

// Initialise the Timer counters
void timerInit(void){ 
	// Timer1
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
	
	// Timer0
	TCCR0A |= (1 << WGM01);	// CTC mode
	/*
		16Mhz / 1024 = 15,625Mhz = 0.000064s = 0.064ms
		target time = 50ms
		50 ms / 0.064ms = 781.25
		0.064ms * 250 = 16ms
		16ms * 3 = 48ms (which is very close to 50ms for debouncing sake!)
		
		16ms / 0.064ms = 250
		(# timer counts) = 250 - 1 = 249
	*/
	TCCR0B |= (1 << CS02) | (1 << CS00);
	TIMSK0 |= (1 << OCIE0A);					// Timer0 interrupt mask (compare)
	OCR0A = 249;		
	TCNT0 = 0;
	TCCR0B = 0;			// Now now the timer isn't actually needed.
}

// Initialising the button press interrupts
// INT0 = select button, D6 = up, D7 = down
void initInt(void){
	PCICR |= (1 << PCIE2);
	PCMSK2 |= (1 << PD5) | (1 << PD6) | (1 << PD7);
}

// ------------------------------------ Other init functions

// Setting up the OLED display
void u8g_setup(void){
	u8g_InitI2C(&u8g, &u8g_dev_ssd1306_128x64_i2c, U8G_I2C_OPT_NONE);
}

// ------------------------------------ Interrupt call functions

// Button press interrupt (now including "select"
// Now working on Timer0 rather than delay.	
ISR(PCINT2_vect){
	
	// First check to see if the timer is waiting for debouncing (i.e. a button has already been pressed).
	if (buttons.anyPressed || stateChanged){ 				// No good doing a button press in the middle of the screen being updated (as I think this is causing problems with the device crashing).
			// If button already pressed, do nothing
	}
	else{
			// Debounding note waiting, so actually do something
		// See what button has been pressed and setup the timer
		if (!(PIND & (1 << PD5))){
				buttons.selectPressed = 1;		
				buttons.anyPressed = 1;
		}
		else if (!(PIND & (1 << PD6))){
				buttons.upPressed = 1;		
				buttons.anyPressed = 1;
		}		

		else if (!(PIND & (1 << PD7))){
				buttons.downPressed = 1;		
				buttons.anyPressed = 1;
		}
		if (buttons.anyPressed){
			// Only one of the buttons we are interseted in was actually pressed
			PCICR = 0;			// Cancel the input change interrupt (only want one button pressed at a time)
			TCNT0 = 0;						// Resetting timer0
			TCCR0B |= (1 << CS02) | (1 << CS00);	// Setting the prescaler (put to zero when not needed)
		}
	}
}

// Timer1 compare interrupt
ISR (TIMER1_COMPA_vect){
	if (lengths < 255){
		// Check is required number of 50ms have pass
		// 10 x 10ms = 1s
		if (ms100++ >= 9){  			
			ms100 = 0;
			lengthTime++;	
			chooseNextState(0);		// Zero means nothing pressed (i.e. time)
			updateScreen = 1;
			totalTime++;
		}
	}
}

// Timer0 compare interrupt (debouncing)
ISR (TIMER0_COMPA_vect){
	// Has debouncing time passed?
	if (++buttons.timer0Ints >= 3){
		// Yes it has
		// Is the button still being pressed down?
		if (buttons.selectPressed && !(PIND & (1 << PD5))){
			TCCR0B = 0;
			TCNT0 = 0;
			PCICR |= (1 << PCIE2);
			chooseNextState(3);
		}
		else if (buttons.upPressed && !(PIND & (1 << PD6))){
			TCCR0B = 0;
			TCNT0 = 0;
			PCICR |= (1 << PCIE2);
			chooseNextState(1);
		}
		else if (buttons.downPressed && !(PIND & (1 << PD7))){
			TCCR0B = 0;
			TCNT0 = 0;
			PCICR |= (1 << PCIE2);
			chooseNextState(2);
		}
		else {
			// Button not actually pressed, 
			TCCR0B = 0;
			TCNT0 = 0;
			PCICR |= (1 << PCIE2);
		}
		// Resetting the buttons struct
			buttons.upPressed = 0;
			buttons.downPressed = 0;
			buttons.selectPressed = 0;
			buttons.debounceMs = 50;
			buttons.timer0Ints = 0;
			buttons.anyPressed = 0;
		//stateChanged = 1;
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
		u8g_SetDefaultForegroundColor(&u8g);
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
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
	//totalTime += lengthTime;
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
	u8g_SetFont(&u8g, u8g_font_6x13);
	do{
		// Draw boxes
		u8g_DrawLine(&u8g, 64, 0, 64, 64);
		u8g_DrawLine(&u8g, 64, 32, 128, 32);
		
		// Draw text
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, "Length"))/2, 5, "Length");
		//u8g_SetFont(&u8g, u8g_font_9x18);
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, str1))/2, 32+5, str1);
		//u8g_SetFont(&u8g, u8g_font_6x13);
		
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, "Total"))/2, 5, "Total");
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, timeToString(totalTime, rS)))/2, 5+15, timeToString(totalTime, rS));
		
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, "Last len"))/2, 5+32, "Last len");
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, timeToString(lastLengthTime, rS)))/2, 5+32+15, timeToString(lastLengthTime, rS));

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
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
		u8g_DrawLine(&u8g, 64, 0, 64, 64);
		u8g_DrawLine(&u8g, 64, 32, 128, 32);
		
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, "Lengths"))/2, 5, "Lengths");
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, str1))/2, 5+20, str1);
		
		
		// Show the first length
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, "Length"))/2, 5, "Length");
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, str3))/2, 5+15, str3);
		
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, "Time"))/2, 32+5, "Time");
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, timeToString(curTime, rS)))/2, 32+5+15, timeToString(curTime, rS));
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
	
	stateChanged = 1;
	switch(currentState){
	case 1:
		switch(event){
		case 1:					// Button 1
			updateMainMenu(1);
			//mainMenuTwo();
			currentState = 2;
			break;
		case 2:					// Button 2
			updateMainMenu(0);
			//mainMenuTwo();
			currentState = 2;
			break;
		case 3:					// Button 3
			currentShowLength = 1;	// Show the first length time when we first start showing
			//showHistory();
			currentState = 3;
			break;
		default:
			//mainMenuOne();
			currentState = 1;
		}
		break;
	case 2:
		switch(event){
		case 1:
			updateMainMenu(1);
			//mainMenuOne();
			currentState = 1;
			break;
		case 2:
			updateMainMenu(0);
			//mainMenuOne();
			currentState = 1;
			break;
		case 3:
			ms100 = 0;
			lengths = 0;
			lengthTime = 0;
			lastLengthTime = 0;
			totalTime = 0;					
			//recordSwim();
			currentState = 4;
			break;
		}
		break;
		// In show history state
	case 3:
		switch(event){
		case 1:
			showPreviousLength();
			//showHistory();
			currentState = 3;
			break;
		case 2:					
			showNextLength();
			//showHistory();
			currentState = 3;
			break;
		case 3:
			up = down = 0;
			menu_current = 0;
			//mainMenuOne();
			currentState = 1;
			break;
		}
		break;
	case 4:
		switch(event){
		case 1:
			up = down = 0;
			menu_current = 1;
			//mainMenuTwo();
			currentState = 2;
			break;
		case 2:
			up = down = 0;
			menu_current = 1;
			//mainMenuTwo();
			currentState = 2;
			break;
		case 3:				
			addLength();
			ms100 = 0;
			//recordSwim();
			currentState = 4;
			break;		
		case 0:				// Time elapsed, so update
			//recordSwim();
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
	buttons.upPressed = 0;
	buttons.downPressed = 0;
	buttons.selectPressed = 0;
	buttons.debounceMs = 50;
	buttons.timer0Ints = 0;
	buttons.anyPressed = 0;
	sei();

	currentState = 1;
	chooseNextState(10);
	stateChanged = 1;
	updateScreen = 1;
	while(1){
		// Updating the display if necessary
		if (stateChanged){
			// Showing the correct display for the current stage
			switch (currentState){				
				case 1:				// Main menu one
					mainMenuOne();
					break;
				case 2:
					mainMenuTwo();
					break;
				case 3:
					showHistory();
					break;
				case 4:
					recordSwim();
					break;					
			}
			stateChanged = 0;
			updateScreen = 0;
		}
	}		

	return (0);
}
