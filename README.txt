5.0

Using function pointers in the FSM model.

Added ability to update time manually which is then uploaded to the FSM.

-------------------------------------------------

3.0

First stage of battery charging and management.

Using the "programmable" MCP73832 to control the current flow to the battery during charging. The signal from this is connected to the uC so I know then the battery if fully charged.

Also using the DS2782 fuel gauge IC. The IC can be communicated to via i2c. I use this to read the current accumulation as a measure of the current capacity of the battery during use and charging.

When the device is being charged, there's a detected "charging display" which will show the current amount. When the battery is fully charged, the current accomulated value is stored as the current max capacity of the battery as this will reduce over time.

----------------------------------------------------------------------------

SportsWatch 2.0

First design of the swim watch pyscially built.

Physical circuit built on perf-board (it and the circuit diagram included).

User interface has been uploaded to be more useful in the pool.

Code has been cleaned up and the interrupt handlers now do as little as possible and all display updates have been moved to the main loop.

----------------------------------------------------------------------------

SportsWatch 1.0
	
Software developed and tested on avr-gcc using avrdude (testing on both Windows 8.1 and Raspberry Pi).

This is still in early development stage, so many bugs and issues with the code.

The program uses u8glib and 24l64 libraries to control the OLED display and EEPROM respectively.

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