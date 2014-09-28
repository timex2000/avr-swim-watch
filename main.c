/*

5.0

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
#define MAIN_MENU_ITEMS 3

#define DS2782EAddress 0b01101000

#define CHARGE PD2
#define OFF PD3
#define UP PD0
#define DOWN PD1
#define CLK PD5
#define SELECT PD4

uint8_t charging = 0;
uint16_t totLenTime = 0;
uint8_t lastLengthTime = 0;
uint8_t currentLength = 0;
uint8_t curLenTime = 0;



typedef enum{
	StateStart,
	StateMenuHistory,
	StateMenuRecord,
	StateMenuSystem,
	StateTimeDateChange,
	StateMenuSystemShow,
	StateSwitchedOff,
	StateShowSystemVersion,
	StateChangeD0,
	StateChangeD1,
	StateChangeMo0,
	StateChangeMo1,
	StateChangeY0,
	StateChangeY1,
	StateChangeH0,
	StateChangeH1,
	StateChangeMi0,
	StateChangeMi1,
	StateChangeS0,
	StateChangeS1,
	StateRecordSwimWait,
	StateRecordSwim,
	StateHistoryOverview,
	StateShowingLength
} state;

typedef enum{
	NoAction,
	StartSystem,
	ShowMenuHistory,
	ShowMenuRecord,
	ShowMenuSystem,
	ShowTimeChangeMenu,
	ShowMenuSystemShow,
	SwitchOffSystem,
	ShowSystemVersion,
	StartD0Change,
	IncreaseD0,
	DecreaseD0,
	StartD1Change,
	IncreaseD1,
	DecreaseD1,
	StartMo0Change,
	IncreaseMo0,
	DecreaseMo0,
	StartMo1Change,
	IncreaseMo1,
	DecreaseMo1,
	StartY0Change,
	IncreaseY0,
	DecreaseY0,
	StartY1Change,
	IncreaseY1,
	DecreaseY1,
	StartH0Change,
	IncreaseH0,
	DecreaseH0,
	StartH1Change,
	IncreaseH1,
	DecreaseH1,
	StartMi0Change,
	IncreaseMi0,
	DecreaseMi0,
	StartMi1Change,
	IncreaseMi1,
	DecreaseMi1,
	StartS0Change,
	IncreaseS0,
	DecreaseS0,
	StartS1Change,
	IncreaseS1,
	DecreaseS1,
	StoreDateTime,
	ShowRecordWait,
	StartRecordingSwim,
	ShowHistoryOverview,
	AddLength,
	AddLastLength,
	UpdateRecordTime,
	ShowFirstLength,
	ShowNextLength,
	ShowPreviousLength
} action;

typedef enum{
	Nothing,
	DownPressed,
	UpPressed,
	SelectPressed,
	BackPressed,
	OffPressed,
	SelectLongPressed,
	ClkPressed
} event;

typedef struct{
	state nextState;
	action actionToDo;
} stateElement;

state currentState = StateStart;
action actionToDo = NoAction;

// FSM state/action matrix
stateElement stateMatrix[24][8] = {
	// Nothing,	DownPressed,	UpPressed,	SelectPressed, BackPressed, OffPressed

	// StateStart
	{{StateMenuHistory, StartSystem}, {StateStart, NoAction}, {StateStart, NoAction}, {StateStart, NoAction}, {StateStart, NoAction}, {StateSwitchedOff, SwitchOffSystem}, {StateStart, NoAction}, {StateStart, NoAction}},
	
	// StateMenuHistory
	{{StateMenuHistory, NoAction}, {StateMenuRecord, ShowMenuRecord}, {StateMenuSystem, ShowMenuSystem}, {StateHistoryOverview, ShowHistoryOverview}, {StateMenuHistory, NoAction}, {StateSwitchedOff, SwitchOffSystem}, {StateMenuHistory, NoAction}, {StateMenuHistory, NoAction}},
	
	// StateMenuRecord
	{{StateMenuRecord, NoAction}, {StateMenuSystem, ShowMenuSystem}, {StateMenuHistory, ShowMenuHistory}, {StateRecordSwimWait, ShowRecordWait}, {StateMenuRecord, NoAction}, {StateSwitchedOff, SwitchOffSystem}, {StateMenuRecord, NoAction}, {StateMenuRecord, NoAction}},
	
	// StateMenuSystem
	{{StateMenuSystem, NoAction}, {StateMenuHistory, ShowMenuHistory}, {StateMenuRecord, ShowMenuRecord}, {StateTimeDateChange, ShowTimeChangeMenu}, {StateMenuSystem, NoAction}, {StateSwitchedOff, SwitchOffSystem}, {StateMenuSystem, NoAction}, {StateMenuSystem, NoAction}},	
	
	// StateTimeDateChange
	{{StateTimeDateChange, NoAction}, {StateMenuSystemShow, ShowMenuSystemShow}, {StateMenuSystemShow, ShowMenuSystemShow}, {StateChangeD0, StartD0Change}, {StateMenuHistory, ShowMenuHistory}, {StateSwitchedOff, SwitchOffSystem}, {StateTimeDateChange, NoAction}, {StateTimeDateChange, NoAction}},
	
	// StateMenuSystemShow
	{{StateMenuSystemShow, NoAction}, {StateTimeDateChange, ShowTimeChangeMenu}, {StateTimeDateChange, ShowTimeChangeMenu}, {StateShowSystemVersion, ShowSystemVersion}, {StateMenuHistory, ShowMenuHistory}, {StateSwitchedOff, SwitchOffSystem}, {StateMenuSystemShow, NoAction}, {StateMenuSystemShow, NoAction}},
	
	// StateSwitchedOff
	{{StateMenuHistory, ShowMenuHistory}, {StateSwitchedOff, NoAction}, {StateSwitchedOff, NoAction}, {StateSwitchedOff, NoAction}, {StateSwitchedOff, NoAction}, {StateSwitchedOff, NoAction}, {StateSwitchedOff, NoAction}, {StateSwitchedOff, NoAction}},
	
	// StateShowSystemVersion
	{{StateShowSystemVersion, NoAction}, {StateShowSystemVersion, NoAction}, {StateShowSystemVersion, NoAction}, {StateShowSystemVersion, NoAction}, {StateMenuSystemShow, ShowMenuSystemShow}, {StateSwitchedOff, NoAction}, {StateShowSystemVersion, NoAction}, {StateShowSystemVersion, NoAction}},
	
	// StateChangeD0
	{{StateChangeD0, NoAction}, {StateChangeD0, IncreaseD0}, {StateChangeD0, DecreaseD0}, {StateChangeD1, StartD1Change}, {StateTimeDateChange, ShowTimeChangeMenu}, {StateSwitchedOff, SwitchOffSystem}, {StateChangeD0, NoAction}, {StateChangeD0, NoAction}},
	
	// StateChangeD1
	{{StateChangeD1, NoAction}, {StateChangeD1, IncreaseD1}, {StateChangeD1, DecreaseD1}, {StateChangeMo0, StartMo0Change}, {StateChangeD0, StartD0Change}, {StateSwitchedOff, SwitchOffSystem}, {StateChangeD1, NoAction}, {StateChangeD1, NoAction}},
	
	// StateChangeMo0
	{{StateChangeMo0, NoAction}, {StateChangeMo0, IncreaseMo0}, {StateChangeMo0, DecreaseMo0}, {StateChangeMo1, StartMo1Change}, {StateChangeD1, StartD1Change}, {StateSwitchedOff, SwitchOffSystem}, {StateChangeMo0, NoAction}, {StateChangeMo0, NoAction}},
	
	// StateChangeMo1
	{{StateChangeMo1, NoAction}, {StateChangeMo1, IncreaseMo1}, {StateChangeMo1, DecreaseMo1}, {StateChangeY0, StartY0Change}, {StateChangeMo0, StartMo0Change}, {StateSwitchedOff, SwitchOffSystem}, {StateChangeMo1, NoAction}, {StateChangeMo1, NoAction}},
	
	// StateChangeY0
	{{StateChangeY0, NoAction}, {StateChangeY0, IncreaseY0}, {StateChangeY0, DecreaseY0}, {StateChangeY1, StartY1Change}, {StateChangeMo1, StartMo1Change}, {StateSwitchedOff, SwitchOffSystem}, {StateChangeY0, NoAction}, {StateChangeY0, NoAction}},
	
	// StateChangeY1
	{{StateChangeY1, NoAction}, {StateChangeY1, IncreaseY1}, {StateChangeY1, DecreaseY1}, {StateChangeH0, StartH0Change}, {StateChangeY0, StartY0Change}, {StateSwitchedOff, SwitchOffSystem}, {StateChangeY1, NoAction}, {StateChangeY1, NoAction}},
	
	
	// StateChangeH0
	{{StateChangeH0, NoAction}, {StateChangeH0, IncreaseH0}, {StateChangeH0, DecreaseH0}, {StateChangeH1, StartH1Change}, {StateChangeY1, StartY1Change}, {StateSwitchedOff, SwitchOffSystem}, {StateChangeH0, NoAction}, {StateChangeH0, NoAction}},
	
	// StateChangeH1
	{{StateChangeH1, NoAction}, {StateChangeH1, IncreaseH1}, {StateChangeH1, DecreaseH1}, {StateChangeMi0, StartMi0Change}, {StateChangeH0, StartH0Change}, {StateSwitchedOff, SwitchOffSystem}, {StateChangeH1, NoAction}, {StateChangeH1, NoAction}},
	
	// StateChangeMi0
	{{StateChangeMi0, NoAction}, {StateChangeMi0, IncreaseMi0}, {StateChangeMi0, DecreaseMi0}, {StateChangeMi1, StartMi1Change}, {StateChangeH1, StartH1Change}, {StateSwitchedOff, SwitchOffSystem}, {StateChangeMi0, NoAction}, {StateChangeMi0, NoAction}},
	
	// StateChangeMi1
	{{StateChangeMi1, NoAction}, {StateChangeMi1, IncreaseMi1}, {StateChangeMi1, DecreaseMi1}, {StateChangeS0, StartS0Change}, {StateChangeMi0, StartMi0Change}, {StateSwitchedOff, SwitchOffSystem}, {StateChangeMi1, NoAction}, {StateChangeMi1, NoAction}},
	
	// StateChangeS0
	{{StateChangeS0, NoAction}, {StateChangeS0, IncreaseS0}, {StateChangeS0, DecreaseS0}, {StateChangeS1, StartS1Change}, {StateChangeMi1, StartMi1Change}, {StateSwitchedOff, SwitchOffSystem}, {StateChangeS0, NoAction}, {StateChangeS0, NoAction}},
	
	// StateChangeS1
	{{StateChangeS1, NoAction}, {StateChangeS1, IncreaseS1}, {StateChangeS1, DecreaseS1}, {StateMenuSystemShow, StoreDateTime}, {StateChangeS0, StartS0Change}, {StateSwitchedOff, SwitchOffSystem}, {StateChangeS1, NoAction}, {StateChangeS1, NoAction}},
	
	// StateRecordSwimWait
	{{StateRecordSwimWait, NoAction}, {StateRecordSwimWait, NoAction}, {StateRecordSwimWait, NoAction}, {StateRecordSwimWait, NoAction}, {StateMenuRecord, ShowMenuRecord}, {StateSwitchedOff, SwitchOffSystem}, {StateRecordSwim, StartRecordingSwim}, {StateRecordSwimWait, NoAction}},
	
	// StateRecordSwim
	{{StateRecordSwim, NoAction}, {StateRecordSwim, NoAction}, {StateRecordSwim, NoAction}, {StateRecordSwim, AddLength}, {StateRecordSwim, NoAction}, {StateRecordSwim, NoAction}, {StateMenuRecord, AddLastLength}, {StateRecordSwim, UpdateRecordTime}},
	
	// StateHistoryOverview
	{{StateHistoryOverview, NoAction}, {StateHistoryOverview, NoAction}, {StateHistoryOverview, NoAction}, {StateShowingLength, ShowFirstLength}, {StateMenuHistory, ShowMenuHistory}, {StateSwitchedOff, SwitchOffSystem}, {StateHistoryOverview, NoAction}, {StateHistoryOverview, NoAction}},
	
	// StateShowingLength
	{{StateShowingLength, NoAction}, {StateShowingLength, ShowPreviousLength}, {StateShowingLength, ShowNextLength}, {StateShowingLength, NoAction}, {StateHistoryOverview, ShowHistoryOverview}, {StateSwitchedOff, SwitchOffSystem}, {StateShowingLength, NoAction}, {StateShowingLength, NoAction}}
	
};

struct lengthHistory_t{
	uint8_t totalLength;
	uint8_t showLength;
} history;

struct timeStruct_t{
	uint8_t h0;
	uint8_t h1;
	uint8_t m0;
	uint8_t m1;
	uint8_t s0;
	uint8_t s1;
} tempTime;

struct dateStruct_t{
	uint8_t d0;
	uint8_t d1;
	uint8_t m0;
	uint8_t m1;
	uint8_t y0;
	uint8_t y1;
} tempDate;

uint8_t tDate[] = {0,0, 0,0, 0,0};
uint8_t tTime[] = {0,0, 0,0, 0,0};

volatile uint8_t pressed[NUMBUTTONS], justPressed[NUMBUTTONS], justReleased[NUMBUTTONS], timePressed[NUMBUTTONS];
u8g_t u8g;
uint16_t vcc = 0;

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

// RTC 1Hz clk output
void startRTC(void){
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

// Initalise the OLED display and blank the screen
void initOLED(void){
	u8g_InitI2C(&u8g, &u8g_dev_ssd1306_128x64_i2c, U8G_I2C_OPT_NONE);
	
	u8g_FirstPage(&u8g);
	do {		
	} while (u8g_NextPage(&u8g));
	_delay_ms(500);
	
	//drawMainMenu(0);
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
	
	//ADCSRA &= ~(1<<ADEN); //Disable ADC
	//ACSR = (1<<ACD); //Disable the analog 
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

void rawDateToChar(char *str, uint8_t day, uint8_t month, uint8_t year){
	str[1] = '0' + (day & 0b00001111);
	str[0] = '0' + ((day >> 4) & 0b00000011);
	str[4] = '0' + (month & 0b00001111);
	str[3] = '0' + ((month >> 4) & 0b00000001);
	str[7] = '0' + (year & 0b00001111);
	str[6] = '0' + ((year >> 4) & 0b00000001);
}

void rawTimeToChar(char *str, uint8_t sec, uint8_t min, uint8_t hour){
	str[1] = '0' + (hour & 0b00001111);
	str[0] = '0' + ((hour >> 4) & 0b00000011);
	str[4] = '0' + (min & 0b00001111);
	str[3] = '0' + ((min >> 4) & 0b00000111);
	str[7] = '0' + (sec & 0b00001111);
	str[6] = '0' + ((sec >> 4) & 0b00000111);
}

void getCurrentTime(uint8_t *hour, uint8_t *min, uint8_t *sec){
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x02);		// VL_seconds register
	TWIStart();
	TWIWrite(0xA2 | 0b00000001);
	*sec = TWIReadNACK();
	TWIStop();
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x03);		// VL_seconds register
	TWIStart();
	TWIWrite(0xA2 | 0b00000001);
	*min = TWIReadNACK();
	TWIStop();
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x04);		// VL_seconds register
	TWIStart();
	TWIWrite(0xA2 | 0b00000001);
	*hour = TWIReadNACK();
	TWIStop();
}

void getCurrentDate(uint8_t *day, uint8_t *month, uint8_t *year){
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x05);		// VL_seconds register
	TWIStart();
	TWIWrite(0xA2 | 0b00000001);
	*day = TWIReadNACK();
	TWIStop();
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x07);		// VL_seconds register
	TWIStart();
	TWIWrite(0xA2 | 0b00000001);
	*month = TWIReadNACK();
	TWIStop();
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x08);		// VL_seconds register
	TWIStart();
	TWIWrite(0xA2 | 0b00000001);
	*year = TWIReadNACK();
	TWIStop();
}

void drawMainMenu(uint8_t menuPosition){
	char *mainMenuStrings[MAIN_MENU_ITEMS] = {"History", "Record", "System"};
	uint8_t i, h;
	u8g_uint_t w, d;
	u8g_SetFont(&u8g, u8g_font_6x13);
	u8g_SetFontRefHeightText(&u8g);
	u8g_SetFontPosTop(&u8g);
	h = u8g_GetFontAscent(&u8g) - u8g_GetFontDescent(&u8g);
	w = u8g_GetWidth(&u8g);
	
	uint8_t rawSec;
	uint8_t rawMin;
	uint8_t rawHour;
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x02);		// VL_seconds register
	TWIStart();
	TWIWrite(0xA2 | 0b00000001);
	rawSec = TWIReadNACK();
	TWIStop();
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x03);		// VL_seconds register
	TWIStart();
	TWIWrite(0xA2 | 0b00000001);
	rawMin = TWIReadNACK();
	TWIStop();
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x04);		// VL_seconds register
	TWIStart();
	TWIWrite(0xA2 | 0b00000001);
	rawHour = TWIReadNACK();
	TWIStop();
	char timeChar[] = "00:00:00";
	//rawSec = 0b01011001;
	rawTimeToChar(timeChar, rawSec, rawMin, rawHour);
	//timeChar = "00:00:00";
	
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
		u8g_DrawStr(&u8g, 60, 50, timeChar);
	} while (u8g_NextPage(&u8g));
}

void showMenuHistory(void){
	drawMainMenu(0);
}

void showMenuRecord(void){
	drawMainMenu(1);
}

void showMenuSystem(void){
	drawMainMenu(2);
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

 

void showOnScreen(void){
	char vccChar[10];
	my_itoa(vcc, vccChar, 10);
	
	u8g_FirstPage(&u8g);
	u8g_SetDefaultForegroundColor(&u8g);
	do {
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
		
		u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, "Switching On"))/2, 10, "Switching On");
		u8g_DrawStr(&u8g, 60, 30, vccChar);
	} while (u8g_NextPage(&u8g));
}

// Screen showing the device is being switched off and what the current battery percentage is
void showOffScreen(void){
	char vccChar[10];
	my_itoa(vcc, vccChar, 10);
	
	u8g_FirstPage(&u8g);
	u8g_SetDefaultForegroundColor(&u8g);
	do {
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
		
		u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, "Switching Off"))/2, 10, "Switching Off");
		u8g_DrawStr(&u8g, 60, 30, vccChar);
	} while (u8g_NextPage(&u8g));
}

void switchOffSystem(void){
	showOffScreen();
	_delay_ms(2000);
	//u8g_SleepOn(&u8g);
	//u8g_SleepOn(&u8g);
	u8g_FirstPage(&u8g);
	do {		
	} while (u8g_NextPage(&u8g));
	u8g_SleepOn(&u8g);
	stopRTC();
	
	EIMSK = (1 << INT1) | (1 << INT0);
	EICRA = (1 << ISC11) | (1 << ISC10) | (1 << ISC01) | (1 << ISC00);
	sei();
	
	
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();	
	
	
  power_twi_disable();
  power_spi_disable();
  power_usart0_disable();
  power_timer0_disable(); //Needed for delay_ms
  power_timer1_disable();
  power_timer2_disable(); //Needed for asynchronous 32kHz operation
	
	sleep_mode();
	//

	sleep_disable();
	EIMSK = 0x00;		// Detach interrupt after waken up.
	EICRA = 0x00;
	
	power_twi_enable();
	power_timer1_enable();
  
	u8g_SleepOff(&u8g);
	u8g_FirstPage(&u8g);
	do {		
	} while (u8g_NextPage(&u8g));
	showOnScreen();
	_delay_ms(2000);
}



// Shows the charging screen 
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
	
	// Getting the time from memory
	char timeStr[] = "00:00:00";
	uint8_t hour, min, sec;
	hour = EEReadByte(5);
	min = EEReadByte(6);
	sec = EEReadByte(7);
	rawTimeToChar(timeStr, sec, min, hour);
	
	// Getting date from memory
	char dateStr[] = "00/00/00";
	uint8_t day, month, year;
	day = EEReadByte(8);
	month = EEReadByte(9);
	year = EEReadByte(10);
	rawDateToChar(dateStr, day, month, year);
	
	u8g_FirstPage(&u8g);
	u8g_SetDefaultForegroundColor(&u8g);
	do {
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
		u8g_DrawLine(&u8g, 64, 0, 64, 64);
		u8g_DrawLine(&u8g, 0, 32, 64, 32);
		
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, "Lengths"))/2, 5, "Lengths");
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, lengthStr))/2, 5+15, lengthStr);
		
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, "Time"))/2, 30+5, "Time");
		u8g_DrawStr(&u8g, (64 - u8g_GetStrWidth(&u8g, totalTimeStr))/2, 30+5+15, totalTimeStr);
		
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, timeStr))/2, 5, dateStr);
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, dateStr))/2, 30+5, timeStr);
		
	} while (u8g_NextPage(&u8g));
}

void showFirstLength(void){
	history.showLength = 0;
	showLength();
}

void showPreviousLength(void){
	getPreviousLength();
	showLength();
}

void showNextLength(void){
	getNextLength();
	showLength();
}

void getPreviousLength(void){
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
	uint8_t curTime = EEReadByte(11+history.showLength);
	
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

void drawSystemMenu(uint8_t menuPosition){
	char *mainMenuStrings[2] = {"Time / Date", "System"};
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
		
		for (i = 0; i < 2; i++){
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
	} while (u8g_NextPage(&u8g));
}

void showTimeDateMenu(void){
	drawSystemMenu(0);
}

// Showing the record screen
void showRecordScreen(void){
	char lengthStr[4], totLenTimeStr[9], curLenTimeStr[9], lastLenTimeStr[9];
	itoa(currentLength, lengthStr, 10);
	timeToString(totLenTime, totLenTimeStr);
	timeToString(curLenTime, curLenTimeStr);
	timeToString(lastLengthTime, lastLenTimeStr);
	

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
		u8g_DrawStr(&u8g, 64+(64 - u8g_GetStrWidth(&u8g, lastLenTimeStr))/2, 5+32+15, lastLenTimeStr);

	} while (u8g_NextPage(&u8g));
}

void recordCurrentLength(void){
	
	lastLengthTime = curLenTime;
	currentLength++;
	// Now recording to EEPROM
	EEWriteByte(2, currentLength);
	EEWriteByte(3, (totLenTime >> 8));
	EEWriteByte(4, (totLenTime << 8) >> 8);
	EEWriteByte(10+currentLength, curLenTime);
	
	curLenTime = 0;
}

// Stores the start of the current session in EEPROM
void timeStampStart(void){
	uint8_t hour, min, sec;
	getCurrentTime(&hour, &min, &sec);
	
	EEWriteByte(5, hour);
	EEWriteByte(6, min);
	EEWriteByte(7, sec);
	
	uint8_t day, month, year;
	getCurrentDate(&day, &month, &year);
	
	EEWriteByte(8, day);
	EEWriteByte(9, month);
	EEWriteByte(10, year);
}


void noAction(void){
}

void startSystem(void){
	initSystem();
	drawMainMenu(0);
}

void showTimeChangeMenu(void){
	drawSystemMenu(0);
}

void showMenuSystemShow(void){
	drawSystemMenu(1);
}

void drawChangeDateTime(uint8_t position){
	// Creating the string required for the display
	char dateChar[] = "00/00/00";
	char timeChar[] = "00:00:00";
	char charTemp[] = "0";
	dateChar[0] += tDate[0];
	dateChar[1] += tDate[1];
	dateChar[3] += tDate[2];
	dateChar[4] += tDate[3];
	dateChar[6] += tDate[4];
	dateChar[7] += tDate[5];
	
	timeChar[0] += tTime[0];
	timeChar[1] += tTime[1];
	timeChar[3] += tTime[2];
	timeChar[4] += tTime[3];
	timeChar[6] += tTime[4];
	timeChar[7] += tTime[5];
	
	//if (position <= 5)
	//	dateChar[position] += tDate[position];
	
	// Drawing the time and date and allowing for change
	u8g_FirstPage(&u8g);
	do{
		if (position <=5){
			// Changing the date

			u8g_DrawStr(&u8g, 5, 5, dateChar);
		}
		else{
			// Changing the time
		
			u8g_SetDefaultForegroundColor(&u8g);			
		
				
			u8g_DrawStr(&u8g, 5, 45, timeChar);
			
		}
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
	} while (u8g_NextPage(&u8g));
}

// For changing the date

void startD0Change(void){
	drawChangeDateTime(0);
}

void increaseD0(void){
	if (++tDate[0] > 3){
		tDate[0] = 0;
	}
	drawChangeDateTime(0);
}

void decreaseD0(void){
	if (tDate[0] == 0){
		tDate[0] = 3;
	}
	else
		tDate[0]--;
	drawChangeDateTime(0);
}

void startD1Change(void){
	drawChangeDateTime(1);
}

void increaseD1(void){
	if (++tDate[1] > 9){
		tDate[1] = 0;
	}
	drawChangeDateTime(1);
}

void decreaseD1(void){
	if (tDate[1] == 0){
		tDate[1] = 9;
	}
	else
		tDate[1]--;
	drawChangeDateTime(1);
}

void startMo0Change(void){
	drawChangeDateTime(2);
}

void increaseMo0(void){
	if (++tDate[2] > 1){
		tDate[2] = 0;
	}
	drawChangeDateTime(2);
}

void decreaseMo0(void){
	if (tDate[2] == 0){
		tDate[2] = 1;
	}
	else
		tDate[2]--;
	drawChangeDateTime(2);
}

void startMo1Change(void){
	drawChangeDateTime(3);
}

void increaseMo1(void){
	if (++tDate[3] > 9){
		tDate[3] = 0;
	}
	drawChangeDateTime(3);
}

void decreaseMo1(void){
	if (tDate[3] == 0){
		tDate[3] = 9;
	}
	else
		tDate[3]--;
	drawChangeDateTime(3);
}

void startY0Change(void){
	drawChangeDateTime(4);
}

void increaseY0(void){
	if (++tDate[4] > 9){
		tDate[4] = 0;
	}
	drawChangeDateTime(4);
}

void decreaseY0(void){
	if (tDate[4] == 0){
		tDate[4] = 9;
	}
	else
		tDate[4]--;
	drawChangeDateTime(4);
}

void startY1Change(void){
	drawChangeDateTime(5);
}

void increaseY1(void){
	if (++tDate[5] > 9){
		tDate[5] = 0;
	}
	drawChangeDateTime(5);
}

void decreaseY1(void){
	if (tDate[5] == 0){
		tDate[5] = 9;
	}
	else
		tDate[5]--;
	drawChangeDateTime(5);
}

// For changing the time
void startH0Change(void){
	drawChangeDateTime(6);
}

void increaseH0(void){
	if (++tTime[0] > 2){
		tTime[0] = 0;
	}
	drawChangeDateTime(6);
}

void decreaseH0(void){
	if (tTime[0] == 0){
		tTime[0] = 2;
	}
	else
		tTime[0]--;
	drawChangeDateTime(6);
}

void startH1Change(void){
	drawChangeDateTime(7);
}

void increaseH1(void){
	if (++tTime[1] > 9){
		tTime[1] = 0;
	}
	drawChangeDateTime(7);
}

void decreaseH1(void){
	if (tTime[1] == 0){
		tTime[1] = 9;
	}
	else
		tTime[1]--;
	drawChangeDateTime(7);
}

void startMi0Change(void){
	drawChangeDateTime(8);
}

void increaseMi0(void){
	if (++tTime[2] > 5){
		tTime[2] = 0;
	}
	drawChangeDateTime(8);
}

void decreaseMi0(void){
	if (tTime[2] == 0){
		tTime[2] = 5;
	}
	else
		tTime[2]--;
	drawChangeDateTime(8);
}

void startMi1Change(void){
	drawChangeDateTime(9);
}

void increaseMi1(void){
	if (++tTime[3] > 9){
		tTime[3] = 0;
	}
	drawChangeDateTime(9);
}

void decreaseMi1(void){
	if (tTime[3] == 0){
		tTime[3] = 9;
	}
	else
		tTime[3]--;
	drawChangeDateTime(9);
}

void startS0Change(void){
	drawChangeDateTime(10);
}

void increaseS0(void){
	if (++tTime[4] > 5){
		tTime[4] = 0;
	}
	drawChangeDateTime(10);
}

void decreaseS0(void){
	if (tTime[4] == 0){
		tTime[4] = 5;
	}
	else
		tTime[4]--;
	drawChangeDateTime(10);
}

void startS1Change(void){
	drawChangeDateTime(11);
}

void increaseS1(void){
	if (++tTime[5] > 9){
		tTime[5] = 0;
	}
	drawChangeDateTime(11);
}

void decreaseS1(void){
	if (tTime[5] == 0){
		tTime[5] = 9;
	}
	else
		tTime[5]--;
	drawChangeDateTime(11);
}


void showSystemVersion(void){
	u8g_FirstPage(&u8g);
	u8g_SetDefaultForegroundColor(&u8g);
	do {
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
		
		// Show the first length
		u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, "SwimWatch"))/2, 5, "Swim Watch");
		u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, "4.3"))/2, 25, "4.3");
		u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, "27 / 09 / 2014"))/2, 45, "27 / 09 / 2014");
	} while (u8g_NextPage(&u8g));
	//_delay_ms(2000);
}

// User has changed time and date, now update to RTC and go back to normal menu
void storeDateTime(void){
	// Storing date into RTC
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x05);
	TWIWrite((tDate[1]&0b00001111) + (tDate[0] << 4));
	TWIStop();
	
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x07);
	TWIWrite((tDate[3]&0b00001111) + (tDate[2] << 4));
	TWIStop();
	
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x08);
	TWIWrite((tDate[5]&0b00001111) + (tDate[4] << 4));
	TWIStop();
	
	// Storing time into RTC
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x04);
	TWIWrite((tTime[1]&0b00001111) + (tTime[0] << 4));
	TWIStop();
	
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x03);
	TWIWrite((tTime[3]&0b00001111) + (tTime[2] << 4));
	TWIStop();
	
	TWIStart();
	TWIWrite(0xA2);
	TWIWrite(0x02);
	TWIWrite((tTime[5]&0b00001111) + (tTime[4] << 4));
	TWIStop();
	

	showTimeChangeMenu();
}

// A simple screen telling the user to long-press select to start session
void showRecordWait(void){
	u8g_FirstPage(&u8g);
	do{
		u8g_SetDefaultForegroundColor(&u8g);
		u8g_DrawStr(&u8g, 5, 5, "Starting new session");
		u8g_DrawStr(&u8g, 5, 25, "Press Select for");
		u8g_DrawStr(&u8g, 5, 45, "4 seconds to start");
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);	
	} while (u8g_NextPage(&u8g));
}

void startRecordingSwim(void){
	totLenTime = 0;
	lastLengthTime = 0;
	curLenTime = 0;
	currentLength = 0;
	startRTC();
	EEOpen();
	// Writing the current time and date to memory
	uint8_t hour, min, sec;
	getCurrentTime(&hour, &min, &sec);
	uint8_t day, month, year;
	getCurrentDate(&day, &month, &year);
	
	EEWriteByte(5, hour);
	EEWriteByte(6, min);
	EEWriteByte(7, sec);
	
	EEWriteByte(8, day);
	EEWriteByte(9, month);
	EEWriteByte(10, year);
	
	// Show record screen
	showRecordScreen();
}

void showHistoryOverview(void){
	showHistoryTotal();
}

void addLength(void){
	recordCurrentLength();
	showRecordScreen();
}

void addLastLength(void){
	//recordCurrentLength();
	showMenuRecord();
	stopRTC();
}

void updateRecordTime(void){
	totLenTime++;
	curLenTime++;
	
	showRecordScreen();
}


int main(void){
	//restoreBatteryEEPROM();
	currentState = StateStart;
	
	// Function pointer test
	void (*actionables[55])(void) = {noAction, startSystem, showMenuHistory, showMenuRecord, showMenuSystem, showTimeChangeMenu, showMenuSystemShow, switchOffSystem, showSystemVersion, startD0Change, increaseD0, decreaseD0, startD1Change, increaseD1, decreaseD1, startMo0Change, increaseMo0, decreaseMo0, startMo1Change, increaseMo1, decreaseMo1, startY0Change, increaseY0, decreaseY0, startY1Change, increaseY1, decreaseY1, startH0Change, increaseH0, decreaseH0, startH1Change, increaseH1, decreaseH1, startMi0Change, increaseMi0, decreaseMi0, startMi1Change, increaseMi1, decreaseMi1, startS0Change, increaseS0, decreaseS0, startS1Change, increaseS1, decreaseS1, storeDateTime, showRecordWait, startRecordingSwim, showHistoryOverview, addLength, addLastLength, updateRecordTime, showFirstLength, showNextLength, showPreviousLength};
	
	while(1){
		event e = getEvent();
		
		// Obtain the next action
		stateElement stateEvaluation = stateMatrix[currentState][e];
		currentState = stateEvaluation.nextState;
		actionToDo = stateEvaluation.actionToDo;
		
		(*actionables[actionToDo])();		
	}
}