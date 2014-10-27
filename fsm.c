#include "fsm.h"

// FSM state/action matrix
stateElement stateMatrix[29][8] = {
	// Nothing,	DownPressed,	UpPressed,	SelectPressed, BackPressed, OffPressed

	// 	StateStart														
 	{{StateMenuHistory, 	 StartSystem}, 	 {StateStart, 	 NoAction}, 	 {StateStart, 	 NoAction}, 	 {StateStart, 	 NoAction}, 	 {StateStart, 	 NoAction}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateStart, 	 NoAction}, 	 {StateStart, 	 NoAction}}, 
 	 															
 	// 	StateMenuHistory														
 	{{StateMenuHistory, 	 NoAction}, 	 {StateMenuRecord, 	 ShowMenuRecord}, 	 {StateMenuSystem, 	 ShowMenuSystem}, 	 {StateHistoryMenuShow, 	 ShowHistoryMenuShow}, 	 {StateMenuHistory, 	 NoAction}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateMenuHistory, 	 NoAction}, 	 {StateMenuHistory, 	 NoAction}}, 
 	 															
 	// 	StateMenuRecord														
 	{{StateMenuRecord, 	 NoAction}, 	 {StateMenuSystem, 	 ShowMenuSystem}, 	 {StateMenuHistory, 	 ShowMenuHistory}, 	 {StateRecordSwimWait, 	 ShowRecordWait}, 	 {StateMenuRecord, 	 NoAction}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateMenuRecord, 	 NoAction}, 	 {StateMenuRecord, 	 NoAction}}, 
 	 															
 	// 	StateMenuSystem														
 	{{StateMenuSystem, 	 NoAction}, 	 {StateMenuHistory, 	 ShowMenuHistory}, 	 {StateMenuRecord, 	 ShowMenuRecord}, 	 {StateTimeDateChange, 	 ShowTimeChangeMenu}, 	 {StateMenuSystem, 	 NoAction}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateMenuSystem, 	 NoAction}, 	 {StateMenuSystem, 	 NoAction}},	 
 	 															
 	// 	StateTimeDateChange														
 	{{StateTimeDateChange, 	 NoAction}, 	 {StateMenuSystemShow, 	 ShowMenuSystemShow}, 	 {StateMenuSystemShow, 	 ShowMenuSystemShow}, 	 {StateChangeD0, 	 StartD0Change}, 	 {StateMenuHistory, 	 ShowMenuHistory}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateTimeDateChange, 	 NoAction}, 	 {StateTimeDateChange, 	 NoAction}}, 
 	 															
 	// 	StateMenuSystemShow														
 	{{StateMenuSystemShow, 	 NoAction}, 	 {StateTimeDateChange, 	 ShowTimeChangeMenu}, 	 {StateTimeDateChange, 	 ShowTimeChangeMenu}, 	 {StateShowSystemVersion, 	 ShowSystemVersion}, 	 {StateMenuHistory, 	 ShowMenuHistory}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateMenuSystemShow, 	 NoAction}, 	 {StateMenuSystemShow, 	 NoAction}}, 
 	 															
 	// 	StateSwitchedOff														
 	{{StateMenuHistory, 	 ShowMenuHistory}, 	 {StateSwitchedOff, 	 NoAction}, 	 {StateSwitchedOff, 	 NoAction}, 	 {StateSwitchedOff, 	 NoAction}, 	 {StateSwitchedOff, 	 NoAction}, 	 {StateSwitchedOff, 	 NoAction}, 	 {StateSwitchedOff, 	 NoAction}, 	 {StateSwitchedOff, 	 NoAction}}, 
 	 															
 	// 	StateShowSystemVersion														
 	{{StateShowSystemVersion, 	 NoAction}, 	 {StateShowSystemVersion, 	 NoAction}, 	 {StateShowSystemVersion, 	 NoAction}, 	 {StateShowSystemVersion, 	 NoAction}, 	 {StateMenuSystemShow, 	 ShowMenuSystemShow}, 	 {StateSwitchedOff, 	 NoAction}, 	 {StateShowSystemVersion, 	 NoAction}, 	 {StateShowSystemVersion, 	 NoAction}}, 
 	 															
 	// 	StateChangeD0														
 	{{StateChangeD0, 	 NoAction}, 	 {StateChangeD0, 	 IncreaseD0}, 	 {StateChangeD0, 	 DecreaseD0}, 	 {StateChangeD1, 	 StartD1Change}, 	 {StateTimeDateChange, 	 ShowTimeChangeMenu}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateChangeD0, 	 NoAction}, 	 {StateChangeD0, 	 NoAction}}, 
 	 															
 	// 	StateChangeD1														
 	{{StateChangeD1, 	 NoAction}, 	 {StateChangeD1, 	 IncreaseD1}, 	 {StateChangeD1, 	 DecreaseD1}, 	 {StateChangeMo0, 	 StartMo0Change}, 	 {StateChangeD0, 	 StartD0Change}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateChangeD1, 	 NoAction}, 	 {StateChangeD1, 	 NoAction}}, 
 	 															
 	// 	StateChangeMo0														
 	{{StateChangeMo0, 	 NoAction}, 	 {StateChangeMo0, 	 IncreaseMo0}, 	 {StateChangeMo0, 	 DecreaseMo0}, 	 {StateChangeMo1, 	 StartMo1Change}, 	 {StateChangeD1, 	 StartD1Change}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateChangeMo0, 	 NoAction}, 	 {StateChangeMo0, 	 NoAction}}, 
 	 															
 	// 	StateChangeMo1														
 	{{StateChangeMo1, 	 NoAction}, 	 {StateChangeMo1, 	 IncreaseMo1}, 	 {StateChangeMo1, 	 DecreaseMo1}, 	 {StateChangeY0, 	 StartY0Change}, 	 {StateChangeMo0, 	 StartMo0Change}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateChangeMo1, 	 NoAction}, 	 {StateChangeMo1, 	 NoAction}}, 
 	 															
 	// 	StateChangeY0														
 	{{StateChangeY0, 	 NoAction}, 	 {StateChangeY0, 	 IncreaseY0}, 	 {StateChangeY0, 	 DecreaseY0}, 	 {StateChangeY1, 	 StartY1Change}, 	 {StateChangeMo1, 	 StartMo1Change}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateChangeY0, 	 NoAction}, 	 {StateChangeY0, 	 NoAction}}, 
 	 															
 	// 	StateChangeY1														
 	{{StateChangeY1, 	 NoAction}, 	 {StateChangeY1, 	 IncreaseY1}, 	 {StateChangeY1, 	 DecreaseY1}, 	 {StateChangeH0, 	 StartH0Change}, 	 {StateChangeY0, 	 StartY0Change}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateChangeY1, 	 NoAction}, 	 {StateChangeY1, 	 NoAction}}, 
 	 															
 	 															
 	// 	StateChangeH0														
 	{{StateChangeH0, 	 NoAction}, 	 {StateChangeH0, 	 IncreaseH0}, 	 {StateChangeH0, 	 DecreaseH0}, 	 {StateChangeH1, 	 StartH1Change}, 	 {StateChangeY1, 	 StartY1Change}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateChangeH0, 	 NoAction}, 	 {StateChangeH0, 	 NoAction}}, 
 	 															
 	// 	StateChangeH1														
 	{{StateChangeH1, 	 NoAction}, 	 {StateChangeH1, 	 IncreaseH1}, 	 {StateChangeH1, 	 DecreaseH1}, 	 {StateChangeMi0, 	 StartMi0Change}, 	 {StateChangeH0, 	 StartH0Change}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateChangeH1, 	 NoAction}, 	 {StateChangeH1, 	 NoAction}}, 
 	 															
 	// 	StateChangeMi0														
 	{{StateChangeMi0, 	 NoAction}, 	 {StateChangeMi0, 	 IncreaseMi0}, 	 {StateChangeMi0, 	 DecreaseMi0}, 	 {StateChangeMi1, 	 StartMi1Change}, 	 {StateChangeH1, 	 StartH1Change}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateChangeMi0, 	 NoAction}, 	 {StateChangeMi0, 	 NoAction}}, 
 	 															
 	// 	StateChangeMi1														
 	{{StateChangeMi1, 	 NoAction}, 	 {StateChangeMi1, 	 IncreaseMi1}, 	 {StateChangeMi1, 	 DecreaseMi1}, 	 {StateChangeS0, 	 StartS0Change}, 	 {StateChangeMi0, 	 StartMi0Change}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateChangeMi1, 	 NoAction}, 	 {StateChangeMi1, 	 NoAction}}, 
 	 															
 	// 	StateChangeS0														
 	{{StateChangeS0, 	 NoAction}, 	 {StateChangeS0, 	 IncreaseS0}, 	 {StateChangeS0, 	 DecreaseS0}, 	 {StateChangeS1, 	 StartS1Change}, 	 {StateChangeMi1, 	 StartMi1Change}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateChangeS0, 	 NoAction}, 	 {StateChangeS0, 	 NoAction}}, 
 	 															
 	// 	StateChangeS1														
 	{{StateChangeS1, 	 NoAction}, 	 {StateChangeS1, 	 IncreaseS1}, 	 {StateChangeS1, 	 DecreaseS1}, 	 {StateMenuSystemShow, 	 StoreDateTime}, 	 {StateChangeS0, 	 StartS0Change}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateChangeS1, 	 NoAction}, 	 {StateChangeS1, 	 NoAction}}, 
 	 															
 	// 	StateRecordSwimWait														
 	{{StateRecordSwimWait, 	 NoAction}, 	 {StateRecordSwimWait, 	 NoAction}, 	 {StateRecordSwimWait, 	 NoAction}, 	 {StateRecordSwimWait, 	 NoAction}, 	 {StateMenuRecord, 	 ShowMenuRecord}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateRecordSwim, 	 StartRecordingSwim}, 	 {StateRecordSwimWait, 	 NoAction}}, 
 	 															
 	// 	StateRecordSwim														
 	{{StateRecordSwim, 	 NoAction}, 	 {StateRecordSwim, 	 NoAction}, 	 {StateRecordSwim, 	 NoAction}, 	 {StateRecordSwim, 	 AddLength}, 	 {StateRecordSwim, 	 NoAction}, 	 {StateRecordSwim, 	 NoAction}, 	 {StateMenuRecord, 	 AddLastLength}, 	 {StateRecordSwim, 	 UpdateRecordTime}}, 
 	 															
 	// 	StateHistoryOverview														
 	{{StateHistoryOverview, 	 NoAction}, 	 {StateHistoryOverview, 	 ShowPreviousSession}, 	 {StateHistoryOverview, 	 ShowNextSession}, 	 {StateShowingLength, 	 ShowFirstLength}, 	 {StateHistoryMenuShow, 	 ShowHistoryMenuShow}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateHistoryOverview, 	 NoAction}, 	 {StateHistoryOverview, 	 NoAction}}, 
 	 															
 	// StateShowingLength														
 	{{StateShowingLength, 	 NoAction}, 	 {StateShowingLength, 	 ShowPreviousLength}, 	 {StateShowingLength, 	 ShowNextLength}, 	 {StateShowingLength, 	 NoAction}, 	 {StateHistoryOverview, 	 ShowHistoryOverview}, 	 {StateSwitchedOff, 	 SwitchOffSystem}, 	 {StateShowingLength, 	 NoAction}, 	 {StateShowingLength, 	NoAction}},

	// StateHistoryMenuShow
	{{StateHistoryMenuShow, NoAction}, {StateHistoryMenuDelete, ShowHistoryMenuDelete}, {StateHistoryMenuDetails, ShowHistoryMenuDetails}, {StateHistoryOverview, ShowLastHistory},  {StateMenuHistory, ShowMenuHistory}, {StateSwitchedOff, 	 SwitchOffSystem}, {StateHistoryMenuShow, NoAction}, {StateHistoryMenuShow, NoAction}},
	
	
	// StateDeleteHistory
	{{StateHistoryMenuDelete, NoAction},{StateHistoryMenuDetails, ShowHistoryMenuDetails}, {StateHistoryMenuShow, ShowHistoryMenuShow}, {StateHistoryDeleteConfirm, ShowHistoryDeleteConfirm},  {StateMenuHistory, ShowMenuHistory}, {StateSwitchedOff, 	 SwitchOffSystem}, {StateHistoryMenuDelete, NoAction}, {StateHistoryMenuDelete, NoAction}},
	
	// StateHistoryDetails
	{{StateHistoryMenuDetails, NoAction}, {StateHistoryMenuShow, ShowHistoryMenuShow}, {StateHistoryMenuDelete, ShowHistoryMenuDelete}, {StateHistoryDetails, ShowHistoryDetails},  {StateMenuHistory, ShowMenuHistory}, {StateSwitchedOff, 	 SwitchOffSystem}, {StateHistoryMenuDetails, NoAction}, {StateHistoryMenuDetails, NoAction}},
	
	// StateHistoryDeleteConfirm
	{{StateHistoryDeleteConfirm, NoAction}, {StateHistoryDeleteConfirm, NoAction}, {StateHistoryDeleteConfirm, NoAction}, {StateHistoryMenuDelete, DeleteHistory}, {StateHistoryMenuDelete, ShowHistoryMenuDelete}, {StateSwitchedOff, SwitchOffSystem}, {StateHistoryDeleteConfirm, NoAction}, {StateHistoryDeleteConfirm, NoAction}},
	
	// StateHistoryDetails
	{{StateHistoryDetails, NoAction}, {StateHistoryDetails, NoAction}, {StateHistoryDetails, NoAction}, {StateHistoryDetails, NoAction}, {StateHistoryMenuDetails, ShowHistoryMenuDetails}, {StateSwitchedOff, SwitchOffSystem}, {StateHistoryDetails, NoAction}, {StateHistoryDetails, NoAction}}
};