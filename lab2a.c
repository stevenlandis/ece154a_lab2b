/*****************************************************************************
* lab2a.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#define AO_LAB2A

#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"

/* Setup state machines */
/**********************************************************************/
static QState Lab2A_initial (Lab2A *me);
static QState Lab2A_on      (Lab2A *me);
static QState Lab2A_Background  (Lab2A *me);
static QState Lab2A_OnScreen  (Lab2A *me);

/**********************************************************************/


Lab2A AO_Lab2A;


void Lab2A_ctor(void)  {
	Lab2A *me = &AO_Lab2A;
	AO_Lab2A.mode = 1;
	AO_Lab2A.draw_mode = 0;
	AO_Lab2A.clear_mode = 0;
	AO_Lab2A.mode_on_screen = 0;

	AO_Lab2A.volume = 0;
	AO_Lab2A.draw_volume = 0;
	AO_Lab2A.clear_mode = 0;
	AO_Lab2A.volume_on_screen = 0;

	QActive_ctor(&me->super, (QStateHandler)&Lab2A_initial);
}


QState Lab2A_initial(Lab2A *me) {
	xil_printf("\n\rInitialization");
    return Q_TRAN(&Lab2A_on);
}

QState Lab2A_on(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("\n\rOn");
			}
			
		case Q_INIT_SIG: {
			return Q_TRAN(&Lab2A_Background);
		}
		
		case ENCODER_UP: {
//			xil_printf("\nEncoder Up from State A");
			if (me->volume < 63) {
				me->volume++;
			}
			me->draw_volume = 1;

			return Q_TRAN(&Lab2A_OnScreen);
		}

		case ENCODER_DOWN: {
//			xil_printf("\nEncoder Down from State A");
			if (me->volume > 0) {
				me->volume--;
			}
			me->draw_volume = 1;

			return Q_TRAN(&Lab2A_OnScreen);
		}

		case ENCODER_CLICK:  {
//			xil_printf("\nChanging State");

			me->volume = 0;
			me->draw_volume = 1;

			return Q_TRAN(&Lab2A_OnScreen);
		}

		case BUTTON_PRESS: {
			me->draw_mode = 1;

			return Q_TRAN(&Lab2A_OnScreen);
		}
	}

	return Q_SUPER(&QHsm_top);
}


/* Create Lab2A_on state and do any initialization code if needed */
/******************************************************************/

// nothing on the screen
QState Lab2A_Background(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
//			xil_printf("\nStartup State A");
			if (me->mode_on_screen) {
				me->clear_mode = 1;
			}

			if (me->volume_on_screen) {
				me->clear_volume = 1;
			}

			return Q_HANDLED();
		}
	}

	return Q_SUPER(&Lab2A_on);

}

QState Lab2A_OnScreen(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {

//			xil_printf("Startup State B\n");
			startTimer();
			return Q_HANDLED();
		}
		case TIMER_END: {

			return Q_TRAN(Lab2A_Background);
		}
	}

	return Q_SUPER(&Lab2A_on);

}

