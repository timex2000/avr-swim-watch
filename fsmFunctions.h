#ifndef _FSMFUNCTIONS_
#define _FSMFUNCTIONS_

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>

void showHistoryDetails(void);
void deleteHistory(void);
void showHistoryDeleteConfirm(void);
void showHistoryMenuShow(void);
void showHistoryMenuDelete(void);
void showHistoryMenuDetails(void);
void showNextSession(void);
void showPreviousSession(void);
void noAction(void);
void updateRecordTime(void);
void addLastLength(void);
void addLength(void);
void showLastHistory(void);
void showHistoryOverview(void);
void startRecordingSwim(void);
void showRecordWait(void);
void storeDateTime(void);
void showSystemVersion(void);
void startD0Change(void);
void increaseD0(void);
void decreaseD0(void);
void startD1Change(void);
void increaseD1(void);
void decreaseD1(void);
void startMo0Change(void);
void increaseMo0(void);
void decreaseMo0(void);
void startMo1Change(void);
void increaseMo1(void);
void decreaseMo1(void);
void startY0Change(void);
void increaseY0(void);
void decreaseY0(void);
void startY1Change(void);
void increaseY1(void);
void decreaseY1(void);
void startH0Change(void);
void increaseH0(void);
void decreaseH0(void);
void startH1Change(void);
void increaseH1(void);
void decreaseH1(void);
void startMi0Change(void);
void increaseMi0(void);
void decreaseMi0(void);
void startMi1Change(void);
void increaseMi1(void);
void decreaseMi1(void);
void startS0Change(void);
void increaseS0(void);
void decreaseS0(void);
void startS1Change(void);
void increaseS1(void);
void decreaseS1(void);

void startSystem(void);
void showTimeChangeMenu(void);
void showMenuSystemShow(void);

void showMenuHistory(void);
void showMenuRecord(void);
void showMenuSystem(void);
void recordCurrentLength(void);
void showRecordScreen(void);
void showTimeDateMenu(void);
void showLength(void);
void showOnScreen(void);
void showOffScreen(void);
void switchOffSystem(void);
void showChargingScreen(void);
void showHistoryTotal(void);
void showFirstLength(void);
void showPreviousLength(void);
void showNextLength(void);
void getPreviousLength(void);
void getNextLength(void);
void rawDateToChar(char *str, uint8_t day, uint8_t month, uint8_t year);
void rawTimeToChar(char *str, uint8_t sec, uint8_t min, uint8_t hour);
void getCurrentTime(uint8_t *hour, uint8_t *min, uint8_t *sec);
void getCurrentDate(uint8_t *day, uint8_t *month, uint8_t *year);
void initOLED(void);
void drawChangeDateTime(uint8_t position);
#endif
