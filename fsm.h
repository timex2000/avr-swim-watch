#ifndef _FSM_H_
#define _FSM_H_

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
	StateShowingLength,
	StateHistoryMenuShow,
	StateHistoryMenuDelete,
	StateHistoryMenuDetails,
	StateHistoryDeleteConfirm,
	StateHistoryDetails
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
	ShowPreviousLength,
	ShowNextSession,
	ShowPreviousSession,
	ShowLastHistory,
	ShowHistoryMenuShow,
	ShowHistoryMenuDelete,
	ShowHistoryMenuDetails,
	ShowHistoryDeleteConfirm,
	DeleteHistory,
	ShowHistoryDetails
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





#endif