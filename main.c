/*
	SwimWatch 7.0
*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
	
#include "u8g.h"
#include "24c64.h"
#include "twi.h"
#include "fsm.h"
#include "menu.h"
#include "fsmFunctions.h"
				
#define NUMBUTTONS 6
#define CHARGE PD2
#define OFF PD3
#define UP PD0
#define DOWN PD1
#define CLK PD5
#define SELECT PD4

extern stateElement stateMatrix[29][8];

volatile uint8_t pressed[NUMBUTTONS], justPressed[NUMBUTTONS], justReleased[NUMBUTTONS], timePressed[NUMBUTTONS];

event getEvent(void){
	event e = Nothing;
	uint8_t longPress = 20;
	
	if (justReleased[2]){
		justReleased[2] = 0;
		e = UpPressed;
	}
	else if (justReleased[3]){
		justReleased[3] = 0;
			e = DownPressed;
	}
	else if (justPressed[5] & (timePressed[5] < longPress) & justReleased[5]){
		justPressed[5] = 0;
		timePressed[5] = 0;
		justReleased[5] = 0;
		e = SelectPressed;
	}
	else if (justReleased[5]){
		justReleased[5] = 0;
	}
	else if (justReleased[0] & (timePressed[0] < longPress)){
		justReleased[0] = 0;
		e = BackPressed;
	}
	else if (pressed[0] & (timePressed[0] > longPress)){
		//justReleased[0] = 0;
		timePressed[0] = 0;
		e = OffPressed;
	}
	else if (pressed[5] & (timePressed[5] > longPress)){
		e = SelectLongPressed;
		timePressed[5] = 0;
		justReleased[5] = 0;
		justPressed[5] = 0;
		//pressed[5] = 0;
	}
	else if (justReleased[4]){
		justReleased[4] = 0;
		e = ClkPressed;
	}
	
	
	
	/*if (pressed[0] & timePressed[0] > 10){
		//justReleased[0] = 0;
		e = OFF_PRESSED;
	}
	else if (justReleased[0] & timePressed[0] < 10){
		justReleased[0] = 0;
		e = BACK_PRESSED;
	}
	else if (justPressed[1]){
		justPressed[1] = 0;
		e = CHARGER_IN;
	}
	else if (justReleased[1]){
		justReleased[1] = 0;
		e = CHARGER_OUT;
	}
	else if (justReleased[2]){
		justReleased[2] = 0;
		e = UP_PRESSED;
	}
	else if (justReleased[3]){
		justReleased[3] = 0;
		e = DOWN_PRESSED;
	}
	else if (justReleased[4]){
		justReleased[4] = 0;
		e = CLK_PRESSED;
	}
	else if (justReleased[5]){
		justReleased[5] = 0;
		e = SELECT_PRESSED;
	}
	*/
	return e;
}


// Initialise timer1 which is used for checking the buttons
void initTimer1(void){
	TCCR1A = (1 << WGM12);
	TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12);				// 64 prescaler
	TIMSK1 = (1 << OCIE1A);
	/* 8MHZ / 64 = 125,000Hz = 0.000008 sec
		(15ms) / (0.000008) = 1875
		(# timer counts) = 1875 - 1 = 1874 */
	OCR1A = 1874;
}

// Initialise the ports
void initPorts(void){
	DDRD = 0x00;
	PORTD = 0xFF;
}

// Sets the number of sessions to zero when the battery is pulled out (might remove this later)
void initMemory(void){
	EEOpen();
	EEWriteByte(2, 0);	// Zero sessions recorded.
}

// Initialise the system
void initSystem(void){
	initPorts();
	initOLED();	
	initTimer1();
	sei();
}

// Converts time into string to be printed in format hh:mm:ss
void timeToString(uint16_t time, char *rS){
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
}

uint8_t divu10(uint8_t n){
		uint16_t q, r;
		
		q = (n >> 1) + (n >> 2);
		q = q + (q >> 4);
		q = q + (q >> 8);
		q = q + (q >> 16);
		q = q >> 3;
		r = n - q*10;
		return q + ((r + 6) >> 4);
}

// Convert uint8_t to char*
// Max value of uint8_t is 255
char* my_uitoa_10(uint8_t value, char *string){
	if (!string)
		return NULL;

	uint8_t c = 0;
	uint8_t size = 0;
	
	if (value >= 100)
		size = 2;
	else if (value >= 10)
		size = 1;
	else
		size = 0;
	string[size+1] = '\0';
	for (uint8_t i = size; i > 0; i--){
		c = value % 10;
		string[i] = '0' + c;
		value /= 10;
		//value = divu10(value);
	}
	string[0] = '0' + value;
			
	return string;	
}

// Checking the inputs for button presses and performing the debouncing
void checkSwitches(void){
	static uint8_t previousState[NUMBUTTONS];
	static uint8_t currentState[NUMBUTTONS];
	static uint16_t lastTime;
	uint8_t index;
	uint8_t input = PIND;
	uint8_t inputCheck[] = {OFF, CHARGE, UP, DOWN, CLK, SELECT};
	
	if (lastTime++ < 4){ // 15ms * 4 debouncing time
		return;
	}
	
	lastTime = 0;
	for (index = 0; index < NUMBUTTONS; index++){		
		if (!(input & (1 << inputCheck[index])))
			currentState[index] = 0;
		else
			currentState[index] = 1;
		
		if (currentState[index] == previousState[index]) {
			if ((pressed[index] == 0) && (currentState[index] == 0)) {
			// just pressed
			justPressed[index] = 1;
			timePressed[index] = 0;
			}
			else if ((pressed[index] == 1) && (currentState[index] == 1)) {
				// just released
				justReleased[index] = 1;
			}
			pressed[index] = !currentState[index];  // remember, digital HIGH means NOT pressed
			if (pressed[index])
				timePressed[index]++;
		}
		previousState[index] = currentState[index];   // keep a running tally of the buttons
	}	
}

// Used to check for button presses and releases
ISR (TIMER1_COMPA_vect){
	checkSwitches();
}

// Used to wake up the device
ISR(INT1_vect){
}

// Used for the charging interrupt
ISR(INT0_vect){
}

int main(void){
	state currentState = StateStart;
	action actionToDo = NoAction;
	
	// Function pointer test
	void (*actionables[64])(void) = {noAction, startSystem, showMenuHistory, showMenuRecord, showMenuSystem, showTimeChangeMenu, showMenuSystemShow, switchOffSystem, showSystemVersion, startD0Change, increaseD0, decreaseD0, startD1Change, increaseD1, decreaseD1, startMo0Change, increaseMo0, decreaseMo0, startMo1Change, increaseMo1, decreaseMo1, startY0Change, increaseY0, decreaseY0, startY1Change, increaseY1, decreaseY1, startH0Change, increaseH0, decreaseH0, startH1Change, increaseH1, decreaseH1, startMi0Change, increaseMi0, decreaseMi0, startMi1Change, increaseMi1, decreaseMi1, startS0Change, increaseS0, decreaseS0, startS1Change, increaseS1, decreaseS1, storeDateTime, showRecordWait, startRecordingSwim, showHistoryOverview, addLength, addLastLength, updateRecordTime, showFirstLength, showNextLength, showPreviousLength, showNextSession, showPreviousSession, showLastHistory, showHistoryMenuShow, showHistoryMenuDelete, showHistoryMenuDetails, showHistoryDeleteConfirm, deleteHistory, showHistoryDetails};
	
	while(1){
		event e = getEvent();
		
		// Obtain the next action
		stateElement stateEvaluation = stateMatrix[currentState][e];
		currentState = stateEvaluation.nextState;
		actionToDo = stateEvaluation.actionToDo;
		
		(*actionables[actionToDo])();		
	}
}