/* fsmFunctions.c"

Contains the functions used by the FSM, with the exception of the menu functions, which are stored in menu.c

*/


#include "fsmFunctions.h"
#include "menu.h"
#include "rtc.h"
#include "twi.h"
#include "24c64.h"
#include "u8g.h"
#include <avr/power.h>
#include <avr/sleep.h>

#define DRAWSTRCTR(x, string) u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, string))/2, x, string)
#define DRAWBOARDER u8g_DrawFrame(&u8g, 0, 0, 128, 64)


u8g_t u8g;
uint16_t totLenTime = 0;
uint8_t lastLengthTime = 0;
uint8_t currentLength = 0;
uint8_t curLenTime = 0;
uint8_t curSessionView = 0;
uint8_t tDate[] = {0,0, 0,0, 0,0};
uint8_t tTime[] = {0,0, 0,0, 0,0};
struct lengthHistory_t{
	uint8_t totalLength;
	uint8_t showLength;
} history;

void showHistoryDetails(void){
	//EEOpen();
	uint8_t histAmount = EEReadByte(2);
	char histString[4];
	my_uitoa_10(histAmount, histString);
	u8g_FirstPage(&u8g);
	do{
		//u8g_DrawStr(&u8g, 5, 5, "Sessions stored:");
		//u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, histString))/2, 30, histString);
		DRAWSTRCTR(5, "Sessions stored");
		DRAWSTRCTR(30, histString);
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);	
	} while (u8g_NextPage(&u8g));
}

void deleteHistory(void){
	curSessionView = 0;
	EEWriteByte(2, 0);
	u8g_FirstPage(&u8g);
	do{
		//u8g_DrawStr(&u8g, 5, 30, "History Deleted!");
		DRAWSTRCTR(5, "History Deleted");
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);	
	} while (u8g_NextPage(&u8g));
	_delay_ms(1000);
	showHistoryMenuDelete();
}

// Asks the user to confirm if they want to delete the history
void showHistoryDeleteConfirm(void){
	u8g_FirstPage(&u8g);
	do{
		//u8g_DrawStr(&u8g, 5, 5, "Press Select");
		DRAWSTRCTR(5, "Press Select");
		DRAWSTRCTR(25, "to delete history");
		//u8g_DrawStr(&u8g, 5, 25, "to delete history");
		//u8g_DrawFrame(&u8g, 0, 0, 128, 64);	
		DRAWBOARDER;
	} while (u8g_NextPage(&u8g));
}


// Showing the different history menus
void showHistoryMenuShow(void){
	showHistoryMenu(0);
}

void showHistoryMenuDelete(void){
	showHistoryMenu(1);
}

void showHistoryMenuDetails(void){
	showHistoryMenu(2);
}

void showNextSession(void){
	if (++curSessionView >= EEReadByte(2)){
		curSessionView = 0;
	}
	showHistoryOverview();
}

void showPreviousSession(void){
	if (EEReadByte(2) != 0){
		if (curSessionView == 0)
			curSessionView = EEReadByte(2)-1;
		else
			curSessionView--;
	}
	else
		curSessionView = 0;
	showHistoryOverview();
}

// Nothing to be done
void noAction(void){
}


void updateRecordTime(void){
	totLenTime++;
	curLenTime++;
	
	showRecordScreen();
}

// Updating details at the end of the current session
void addLastLength(void){
	//recordCurrentLength();
	showMenuRecord();
	//uint8_t sessionsTemp = EEReadByte(2);
	//if (sessionsTemp == 0);
	
	curSessionView++;
	EEWriteByte(2, curSessionView);
	stopRTC();
}

// Adding a length to the current session
void addLength(void){
	recordCurrentLength();
	showRecordScreen();
}

// Loads the last session stored in memory
void showLastHistory(void){
	curSessionView = EEReadByte(2);
	if (curSessionView != 0)
		curSessionView--;
	showHistoryTotal();
}

// Shows the overall history screen
void showHistoryOverview(void){
	showHistoryTotal();
}

// Starts recording the current swimming session (gets time, etc.)
void startRecordingSwim(void){
	totLenTime = 0;
	lastLengthTime = 0;
	curLenTime = 0;
	currentLength = 0;
	
	EEOpen();
	curSessionView = EEReadByte(2);
	// Writing the current time and date to memory
	uint8_t hour, min, sec;
	getCurrentTime(&hour, &min, &sec);
	uint8_t day, month, year;
	getCurrentDate(&day, &month, &year);
	
	EEWriteByte(3+(curSessionView*255)+0, hour);
	EEWriteByte(3+(curSessionView*255)+1, min);
	EEWriteByte(3+(curSessionView*255)+2, sec);
	
	EEWriteByte(3+(curSessionView*255)+3, day);
	EEWriteByte(3+(curSessionView*255)+4, month);
	EEWriteByte(3+(curSessionView*255)+5, year);
	
	// Show record screen
	startRTC();
	showRecordScreen();
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

// Shows the current system version and compilation date
void showSystemVersion(void){
	u8g_FirstPage(&u8g);
	u8g_SetDefaultForegroundColor(&u8g);
	do {
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);
		
		// Show the first length
		u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, "SwimWatch"))/2, 5, "Swim Watch");
		u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, "6.0"))/2, 25, "6.0");
		u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, "04/10/2014"))/2, 45, "04/10/2014");
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

// Screen used for changing the time and date.
void drawChangeDateTime(uint8_t position){
	// Creating the string required for the display
	char dateChar[] = "00/00/00";
	char timeChar[] = "00:00:00";
	
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


void showMenuHistory(void){
	drawMainMenu(0);
}

void showMenuRecord(void){
	drawMainMenu(1);
}

void showMenuSystem(void){
	drawMainMenu(2);
}

// Record the current length in memory
void recordCurrentLength(void){
	
	lastLengthTime = curLenTime;
	currentLength++;
	// Now recording to EEPROM
	EEWriteByte(3+(curSessionView*255)+6, currentLength);
	EEWriteByte(3+(curSessionView*255)+7, (totLenTime >> 8));
	EEWriteByte(3+(curSessionView*255)+8, (totLenTime << 8) >> 8);
	EEWriteByte(3+(curSessionView*255)+9+currentLength-1, curLenTime);
	
	curLenTime = 0;
}


// Showing the record screen
void showRecordScreen(void){
	char lengthStr[4], totLenTimeStr[9], curLenTimeStr[9], lastLenTimeStr[9];
	my_uitoa_10(currentLength, lengthStr);
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


void showTimeDateMenu(void){
	drawSystemMenu(0);
}

// Show the selected length for the current session
void showLength(void){
	uint8_t curTime = EEReadByte(3+(curSessionView*255)+9+history.showLength);
	
	char lengthStr[4], timeStr[9];
	my_uitoa_10(history.showLength+1, lengthStr);
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

 

void showOnScreen(void){
	uint8_t vcc = getBatteryPercentage();
	char vccChar[10];
	my_uitoa_10(vcc, vccChar);
	
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
	uint8_t vcc = getBatteryPercentage();
	char vccChar[10];
	my_uitoa_10(vcc, vccChar);
	
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
	uint8_t vcc = getBatteryPercentage();
	char vccChar[10];
	my_uitoa_10(vcc, vccChar);
	
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
	//EEOpen();
	
	if (EEReadByte(2) == 0){
		u8g_FirstPage(&u8g);
		u8g_SetDefaultForegroundColor(&u8g);
		do {
			u8g_DrawFrame(&u8g, 0, 0, 128, 64);
			
			// Show the first length
			u8g_DrawStr(&u8g, (124 - u8g_GetStrWidth(&u8g, "No History"))/2, 25, "No History");
		} while (u8g_NextPage(&u8g));
	}
	else{
	
		uint8_t lengths = EEReadByte(3+(curSessionView*255)+6);
		history.totalLength = lengths;
		history.showLength = 255;
		// This will be wrong till I update the record length
		uint16_t totalTime = EEReadByte(3+(curSessionView*255)+7) << 8;
		totalTime += EEReadByte(3+(curSessionView*255)+8);
		
		history.showLength = 255;
		history.totalLength = lengths;
		
		// Creating the times for the history display
		char lengthStr[4], totalTimeStr[10];	
		my_uitoa_10(lengths, lengthStr);
		timeToString(totalTime, totalTimeStr);
		
		// Getting the time from memory
		char timeStr[] = "00:00:00";
		uint8_t hour, min, sec;
		hour = EEReadByte(3+(curSessionView*255)+0);
		min = EEReadByte(3+(curSessionView*255)+1);
		sec = EEReadByte(3+(curSessionView*255)+2);
		rawTimeToChar(timeStr, sec, min, hour);
		
		// Getting date from memory
		char dateStr[] = "00/00/00";
		uint8_t day, month, year;
		day = EEReadByte(3+(curSessionView*255)+3);
		month = EEReadByte(3+(curSessionView*255)+4);
		year = EEReadByte(3+(curSessionView*255)+5);
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

// Initalise the OLED display and blank the screen
void initOLED(void){
	u8g_InitI2C(&u8g, &u8g_dev_ssd1306_128x64_i2c, U8G_I2C_OPT_NONE);
	
	u8g_FirstPage(&u8g);
	do {		
	} while (u8g_NextPage(&u8g));
	_delay_ms(500);
	
	//drawMainMenu(0);
}