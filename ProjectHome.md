Using the AVR line, this project intends to develop the hardware and software for an Open Source "sports watch" that can be used to count swim length, showing the current swim length and time, and allowing for post swim analysis.

Version 5.0

  * FSM model updated to use function pointers
  * Ability to change the date and time on the RTC added
  * Long-press Off and Select added to allow easier movement through menus.
  * Time and date recorded for each session.


---

Version 3.3

  * FSM model updated
  * Charging and fuel-gauge added
  * RTC added for keeping time of lengths
  * 4 buttons now used.


---

Version 2.0

Prototype build on perf-board for testing in pool.

User interfaces are cleaned up.

Code cleaned up so that interrupt handlers are doing as little as possible and all display updates are done in the main loop.


---

Version 1.0

First full draft version of the swim watch.

Can record up to 255 lengths with up to about 4 minutes per length (seems long enough to me). Once recorded, one previous swim can be shown on the display, which will show the number of lengths and the time taken for each length (scrollable by pressing up / down).

Only developed on breadboard.