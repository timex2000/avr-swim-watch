#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "rtc.h"
#include "twi.h"

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
