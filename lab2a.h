/*****************************************************************************
* lab2a.h for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#ifndef lab2a_h
#define lab2a_h

enum Lab2ASignals {
	ENCODER_UP = Q_USER_SIG,
	ENCODER_DOWN,
	ENCODER_CLICK,
	TIMER_END,
	BUTTON_PRESS
};


extern struct Lab2ATag AO_Lab2A;
typedef struct Lab2ATag  {               //Lab2A State machine
	QActive super;

	int volume;
	int draw_volume;
	int clear_volume;
	int volume_on_screen;

	int mode;
	int draw_mode;
	int clear_mode;
	int mode_on_screen;

}  Lab2A;

void Lab2A_ctor(void);
void ButtonHandler(void *CallbackRef);
void TwistHandler(void *CallbackRef);
void TimerHandler(void * CallbackRef);

#endif  
