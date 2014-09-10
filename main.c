/*

3.3

Re-wrote the program from scratch using a better FSM for all states and events. THe interrupts are connected to power-off and charging pins, which are used to start the device if it's switched off.	

*/

#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include <avr/power.h>
#include <avr/sleep.h>
	
#include "u8g.h"
#include "24c64.h"
#include "twi.h"

#define NUMBUTTONS 6
#define MAIN_MENU_ITEMS 2

#define DS2782EAddress 0b01101000

#define CHARGE PD2
#define OFF PD3
#define UP PD0
#define DOWN PD1
#define CLK PD5
#define SELECT PD4

uint8_t charging = 0;
uint16_t totLenTime = 0;
uint8_t currentLength = 0;
uint8_t curLenTime = 0;

struct lengthHistory_t{
	uint8_t totalLength;
	uint8_t showLength;
} history;

typedef enum{
	STATE_MM1,
	STATE_OFF,
	STATE_CHARGING,
	STATE_MM2,
	STATE_HISTORY,
	STATE_SHOWLAP,
	STATE_RECORD
} state;

typedef enum{
	NO_ACTION,
	SHOW_MM1,
	SWITCH_OFF,
	CHARGING,
	SHOW_MM2,
	UPDATE_CHARGE,
	SHOW_HISTORY,
	SHOW_NEXT_LENGTH,
	SHOW_PREV_LENGTH,
	SHOW_RECORD,
	UPDATE_RECORD,
	ADD_LENGTH
} action;

typedef enum{
	NOTHING,
	OFF_PRESSED,
	CHARGER_IN,
	CHARGER_OUT,
	DOWN_PRESSED,
	UP_PRESSED,
	CLK_PRESSED,
	SELECT_PRESSED
} event;

typedef struct{
	state nextState;
	action actionToDo;
} stateElement;

volatile uint8_t pressed[NUMBUTTONS], justPressed[NUMBUTTONS], justReleased[NUMBUTTONS];
u8g_t u8g;
uint16_t vcc = 0;

event getEvent(){
	event e = NOTHING;
	
	if (justPressed[0]){
		justPressed[0] = 0;
		e = OFF_PRESSED;
	}
	else if (justPressed[1]){
		justPressed[1] = 0;
		e = CHARGER_IN;
	}
	else if (justReleased[1]){
		justReleased[1] = 0;
		e = CHARGER_OUT;
	}
	else if (justPressed[2]){
		justPressed[2] = 0;
		e = UP_PRESSED;
	}
	else if (justPressed[3]){
		justPressed[3] = 0;
		e = DOWN_PRESSED;
	}
	else if (justPressed[4]){
		justPressed[4] = 0;
		e = CLK_PRESSED;
	}
	else if (justPressed[5]){
		justPressed[5] = 0;
		e = SELECT_PRESSED;
	}
	
	return e;
}

// RTC 1Hz clk output
void initTimer(void){
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x0D);
	TWIWrite(0b10000011);
	TWIStop();
}

// Stops the CLK output
void stopRTC(void){
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x0D);
	TWIWrite(0x00);			
	TWIStop();
}

void initOLED(void){
	u8g_InitI2C(&u8g, &u8g_dev_ssd1306_128x64_i2c, U8G_I2C_OPT_NONE);
}

void initTimer1(void){
	TCCR1A = (1 << WGM12);
	TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12);				// 64 prescaler
	TIMSK1 = (1 << OCIE1A);
	/* 8MHZ / 64 = 125,000Hz = 0.000008 sec
		(15ms) / (0.000008) = 1875
		(# timer counts) = 1875 - 1 = 1874 */
	OCR1A = 1874;
}

void initPorts(void){
	DDRD = 0x00;
	PORTD = 0xFF;
}

void initSystem(void){
	initPorts();
	initOLED();	
	initTimer1();
	sei();
}

// Converts time into string to be printed in format hh:mm:ss
void timeToString(uint16_t time, char *rS){
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
}

// Gets the current percentage of the battery
void getBatteryPercentage(void){
	TWIStart();
	TWIWrite(DS2782EAddress);
			// Average current
	TWIWrite(0x10);				// Current accumulation
	
	TWIStart();
	//Check status
	
	TWIWrite(DS2782EAddress|0b00000001);

	vcc = TWIReadACK() << 8;//}
	vcc += TWIReadNACK();
	
	vcc = vcc*(0.3125);	// 6.25uVh/Rsns = 6.25u/20mOhm = 0.0003125Ah = 0.3125mAh = 1/3.2 = 312uA
	
	vcc = (vcc * 100) / 300;		// Converting to % (assumming 373mAh = 100% from experiment

	TWIStop();
}

char* my_itoa(uint16_t value, char *string, int radix) 
{ 
   if(!string) 
   { 
      return NULL; 
   } 

   if(0 != value) 
   { 
      char scratch[34]; 
      int neg; 
      int offset; 
      int c; 
      unsigned int accum; 

      offset =32; 
      scratch[33] = '\0'; 

      if(radix == 10 && value < 0) 
      { 
         neg = 1; 
         accum = -value; 
      } 
      else 
      { 
         neg = 0; 
         accum = (unsigned int)value; 
      } 

      while(accum) 
      { 
         c = accum % radix; 
         if(c < 10) 
         { 
            c += '0'; 
         } 
         else 
         { 
            c += 'A' - 10; 
         } 
         scratch[offset] = c; 
         accum /= radix; 
         offset--; 
      } 
       
      if(neg) 
      { 
         scratch[offset] = '-'; 
      } 
      else 
      { 
         offset++; 
      } 

      strcpy(string,&scratch[offset]); 
   } 
   else 
   { 
      string[0] = '0'; 
      string[1] = '\0'; 
   } 

   return string;
}

void drawMainMenu(uint8_t menuPosition){
	char *mainMenuStrings[MAIN_MENU_ITEMS] = {"Last swim", "New Swim"};
	uint8_t i, h;
	u8g_uint_t w, d;
	u8g_SetFont(&u8g, u8g_font_6x13);
	u8g_SetFontRefHeightText(&u8g);
	u8g_SetFontPosTop(&u8g);
	h = u8g_GetFontAscent(&u8g) - u8g_GetFontDescent(&u8g);
	w = u8g_GetWidth(&u8g);
	
	char vccChar[10];
	my_itoa(vcc, vccChar, 10);

	// Doing the actual drawing
	u8g_FirstPage(&u8g);
	do{
		
		for (i = 0; i < MAIN_MENU_ITEMS; i++){
			d = (w - u8g_GetStrWidth(&u8g, mainMenuStrings[i])) / 2;
			u8g_SetDefaultForegroundColor(&u8g);
			if (i == menuPosition){
				u8g_DrawBox(&u8g, 0, i*h+1, w, h);
				u8g_SetDefaultBackgroundColor(&u8g);
			}
			u8g_DrawStr(&u8g, d, i*h, mainMenuStrings[i]);
		}
		u8g_SetDefaultForegroundColor(&u8g);
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
		
		u8g_DrawStr(&u8g, 5, 50, vccChar);
	} while (u8g_NextPage(&u8g));
}

void showMM1(void){
	drawMainMenu(0);
}

void showMM2(void){
	drawMainMenu(1);
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
			}
			else if ((pressed[index] == 1) && (currentState[index] == 1)) {
				// just released
				justReleased[index] = 1;
			}
			pressed[index] = !currentState[index];  // remember, digital HIGH means NOT pressed
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

 

void showOnScreen(void){
}

void switchOff(void){
	//u8g_SleepOn(&u8g);
	//u8g_SleepOn(&u8g);
	u8g_FirstPage(&u8g);
	do {		
	} while (u8g_NextPage(&u8g));
	
	EIMSK = (1 << INT1) | (1 << INT0);
	EICRA = (1 << ISC11) | (1 << ISC10) | (1 << ISC01) | (1 << ISC00);
	sei();
	
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();	
	sleep_mode();
	//_delay_ms(100);

	sleep_disable();
	EIMSK = 0x00;		// Detach interrupt after waken up.
	EICRA = 0x00;
	//u8g_SleepOff(&u8g);
	_delay_ms(500);
}

void showOffScreen(void){
	char vccChar[10];
	my_itoa(vcc, vccChar, 10);
	
	u8g_FirstPage(&u8g);
	u8g_SetDefaultForegroundColor(&u8g);
	do {
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
		
		// Show the first length
		u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, "Switching Off"))/2, 10, "Switching Off");
		u8g_DrawStr(&u8g, 60, 30, vccChar);
	} while (u8g_NextPage(&u8g));
}

void showChargingScreen(void){
	char vccChar[10];
	my_itoa(vcc, vccChar, 10);
	
	u8g_FirstPage(&u8g);
	u8g_SetDefaultForegroundColor(&u8g);
	do {
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
		
		// Show the first length
		u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, "Charging"))/2, 5, "Charging");
		u8g_DrawStr(&u8g, 60, 20, vccChar);
	} while (u8g_NextPage(&u8g));
}

void showHistoryTotal(void){
	EEOpen();
	
	uint8_t lengths = EEReadByte(2);
	history.totalLength = lengths;
	history.showLength = 255;
	// This will be wrong till I update the record length
	uint16_t totalTime = EEReadByte(3) << 8;
	totalTime += EEReadByte(4);
	
	// Creating the times for the history display
	char lengthStr[4], totalTimeStr[10];	
	itoa(lengths, lengthStr, 10);
	timeToString(totalTime, totalTimeStr);
	
	u8g_FirstPage(&u8g);
	u8g_SetDefaultForegroundColor(&u8g);
	do {
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
		u8g_DrawLine(&u8g, 64, 0, 64, 64);
		
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, "Lengths"))/2, 5, "Lengths");
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, lengthStr))/2, 5+20, lengthStr);
		
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, "Time"))/2, 5, "Time");
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, totalTimeStr))/2, 5+20, totalTimeStr);
		
	} while (u8g_NextPage(&u8g));
}

void getPreviousLength(){
	if (history.showLength == 255)
		history.showLength = 0;
	else if (history.showLength == 0)
		history.showLength = history.totalLength-1;
	else
		history.showLength--;
}

void getNextLength(void){
	if (history.showLength == 255)
		history.showLength = 0;
	else if (++history.showLength >= history.totalLength)
		history.showLength = 0;
}

void showLength(void){
	uint8_t curTime = EEReadByte(5+history.showLength);
	
	char lengthStr[4], timeStr[9];
	itoa(history.showLength+1, lengthStr, 10);
	timeToString(curTime, timeStr);
	
	u8g_FirstPage(&u8g);
	u8g_SetDefaultForegroundColor(&u8g);
	do {
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
		u8g_DrawLine(&u8g, 64, 0, 64, 64);
		
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, "Length"))/2, 5, "Length");
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, lengthStr))/2, 5+20, lengthStr);
		
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, "Time"))/2, 5, "Time");
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, timeStr))/2, 5+20, timeStr);
		
	} while (u8g_NextPage(&u8g));
}

// Showing the record screen
void showRecordScreen(void){
	char lengthStr[4], totLenTimeStr[9], curLenTimeStr[9];
	itoa(currentLength, lengthStr, 10);
	timeToString(totLenTime, totLenTimeStr);
	timeToString(curLenTime, curLenTimeStr);

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
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, lengthStr))/2, 32+5, lengthStr);
		
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, "Total"))/2, 5, "Total");
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, totLenTimeStr))/2, 5+15, totLenTimeStr);
		
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, "Last len"))/2, 5+32, "Last len");
		//u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, timeToString(lastLengthTime, rS)))/2, 5+32+15, timeToString(lastLengthTime, rS));

	} while (u8g_NextPage(&u8g));
}

void recordCurrentLength(void){
	
	currentLength++;
	// Now recording to EEPROM
	EEWriteByte(2, currentLength);
	EEWriteByte(3, (totLenTime >> 8));
	EEWriteByte(4, (totLenTime << 8) >> 8);
	EEWriteByte(4+currentLength, curLenTime);
	
	curLenTime = 0;
}

int main(void){
	state currentState = STATE_MM1;
	action actionToDo = NO_ACTION;
	
	// FSM state/action matrix
	stateElement stateMatrix[7][8] = {
		// MM1
		{{STATE_MM1, NO_ACTION}, {STATE_OFF, SWITCH_OFF}, {STATE_CHARGING, CHARGING}, {STATE_MM1, NO_ACTION}, {STATE_MM2, SHOW_MM2}, {STATE_MM2, SHOW_MM2}, {STATE_MM1, NO_ACTION}, {STATE_HISTORY, SHOW_HISTORY}},
		
		// OFF
		{{STATE_MM1, SHOW_MM1}, {STATE_OFF, NO_ACTION}, {STATE_OFF, NO_ACTION}, {STATE_OFF, NO_ACTION}, {STATE_OFF, NO_ACTION}, {STATE_OFF, NO_ACTION}, {STATE_OFF, NO_ACTION}, {STATE_OFF, NO_ACTION}},
		
		// CHARING
		{{STATE_CHARGING, NO_ACTION}, {STATE_CHARGING, NO_ACTION}, {STATE_CHARGING, NO_ACTION}, {STATE_MM1, SHOW_MM1}, {STATE_CHARGING, NO_ACTION}, {STATE_CHARGING, NO_ACTION}, {STATE_CHARGING, UPDATE_CHARGE}, {STATE_CHARGING, NO_ACTION}},
		
		// MM2
		{{STATE_MM2, NO_ACTION}, {STATE_OFF, SWITCH_OFF}, {STATE_CHARGING, CHARGING}, {STATE_MM2, NO_ACTION}, {STATE_MM1, SHOW_MM1}, {STATE_MM1, SHOW_MM1}, {STATE_MM2, NO_ACTION}, {STATE_RECORD, SHOW_RECORD}},
		
		// HISTORY
		{{STATE_HISTORY, NO_ACTION}, {STATE_OFF, SWITCH_OFF}, {STATE_CHARGING, CHARGING}, {STATE_HISTORY, NO_ACTION}, {STATE_SHOWLAP, SHOW_NEXT_LENGTH}, {STATE_SHOWLAP, SHOW_PREV_LENGTH}, {STATE_HISTORY, NO_ACTION}, {STATE_MM1, SHOW_MM1}},
		
		// SHOWLAP
		{{STATE_SHOWLAP, NO_ACTION}, {STATE_OFF, SWITCH_OFF}, {STATE_CHARGING, CHARGING}, {STATE_HISTORY, NO_ACTION}, {STATE_SHOWLAP, SHOW_NEXT_LENGTH}, {STATE_SHOWLAP, SHOW_PREV_LENGTH}, {STATE_HISTORY, NO_ACTION}, {STATE_HISTORY, SHOW_HISTORY}},
		
		// RECORD
		{{STATE_RECORD, NO_ACTION}, {STATE_OFF, SWITCH_OFF}, {STATE_CHARGING, CHARGING}, {STATE_RECORD, NO_ACTION}, {STATE_MM1, SHOW_MM1}, {STATE_MM1, SHOW_MM1}, {STATE_RECORD, UPDATE_RECORD}, {STATE_RECORD, ADD_LENGTH}}
	};
	
	initSystem();
	showMM1();
	
	while(1){
		event e = getEvent();
		
		// Obtain the next action
		stateElement stateEvaluation = stateMatrix[currentState][e];
		currentState = stateEvaluation.nextState;
		actionToDo = stateEvaluation.actionToDo;
		
		switch(actionToDo){
			case NO_ACTION:
				break;
			case SHOW_MM1:
				getBatteryPercentage();
				showMM1();
				break;
			case SWITCH_OFF:
				getBatteryPercentage();
				showOffScreen();
				_delay_ms(2000);
				switchOff();
				break;
			case CHARGING:
				initTimer();
				getBatteryPercentage();
				showChargingScreen();
				break;
			case SHOW_MM2:
				getBatteryPercentage();
				showMM2();
				break;
			case UPDATE_CHARGE:
				getBatteryPercentage();
				showChargingScreen();
				break;
			case SHOW_HISTORY:
				showHistoryTotal();
				break;
			case SHOW_NEXT_LENGTH:
				getNextLength();
				showLength();
				break;
			case SHOW_PREV_LENGTH:
				getPreviousLength();
				showLength();
				break;
			case SHOW_RECORD:
				initTimer();
				currentLength = 0;
				totLenTime = 0;
				showRecordScreen();
				break;
			case UPDATE_RECORD:
				totLenTime++;
				curLenTime++;
				showRecordScreen();
				break;
			case ADD_LENGTH:
				//curLenTime = 0;
				recordCurrentLength();
				showRecordScreen();
				break;
			default:
				break;
		}
	}
}