#ifndef _MENU_H_
#define _MENU_H_

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>

uint8_t getBatteryPercentage(void);
void drawSystemMenu(uint8_t menuPosition);
void drawMainMenu(uint8_t menuPosition);
void showHistoryMenu(uint8_t position);


#endif