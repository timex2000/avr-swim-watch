#include "menu.h"
#include "u8g.h"
#include "twi.h"


#define DS2782EAddress 0b01101000

extern u8g_t u8g;
#define MAIN_MENU_ITEMS 3

// Gets the current percentage of the battery
uint8_t getBatteryPercentage(void){
	TWIStart();
	TWIWrite(DS2782EAddress);
			// Average current
	TWIWrite(0x10);				// Current accumulation
	
	TWIStart();
	//Check status
	
	TWIWrite(DS2782EAddress|0b00000001);
	
	uint8_t vcc;

	vcc = TWIReadACK() << 8;//}
	vcc += TWIReadNACK();
	
	vcc = vcc*(0.3125);	// 6.25uVh/Rsns = 6.25u/20mOhm = 0.0003125Ah = 0.3125mAh = 1/3.2 = 312uA
	
	vcc = (vcc * 100) / 300;		// Converting to % (assumming 373mAh = 100% from experiment

	TWIStop();
	return vcc;
}

// Draws the main menu
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
	
	uint8_t vcc = getBatteryPercentage();
	char vccChar[4] = "000";
	my_uitoa_10(vcc, vccChar);

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

void showHistoryMenu(uint8_t position){
	char *mainMenuStrings[3] = {"Show", "Delete", "Details"};
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
		
		for (i = 0; i < 3; i++){
			d = (w - u8g_GetStrWidth(&u8g, mainMenuStrings[i])) / 2;
			u8g_SetDefaultForegroundColor(&u8g);
			if (i == position){
				u8g_DrawBox(&u8g, 0, i*h+1, w, h);
				u8g_SetDefaultBackgroundColor(&u8g);
			}
			u8g_DrawStr(&u8g, d, i*h, mainMenuStrings[i]);
		}
		u8g_SetDefaultForegroundColor(&u8g);
		u8g_DrawFrame(&u8g, 0, 0, 128, 64);	
	} while (u8g_NextPage(&u8g));
}

