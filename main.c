/*****************************************************************************
* main.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#include "qpn_port.h"                                       /* QP-nano port */
#include "bsp.h"                             /* Board Support Package (BSP) */
#include "lab2a.h"                               /* application interface */
#include "lcd.h"



static QEvent l_lab2aQueue[30];

QActiveCB const Q_ROM Q_ROM_VAR QF_active[] = {
	{ (QActive *)0,            (QEvent *)0,          0                    },
	{ (QActive *)&AO_Lab2A,    l_lab2aQueue,         Q_DIM(l_lab2aQueue)  }
};

Q_ASSERT_COMPILE(QF_MAX_ACTIVE == Q_DIM(QF_active) - 1);

// Do not edit main, unless you have a really good reason
int main(void) {
	xil_printf("Starting\n");
	Lab2A_ctor(); // inside of lab2a.c
	BSP_init(); // inside of bsp.c, starts out empty!

	//fillBackground(0,0,239,319);
	//fillBackground(0,0,239,319);
	//drawVolume(20);
	//drawMode(5);
	//setColor(255,0,0);
	//fillRect(0,0,40,40);
	QF_run(); // inside of qfn.c
	xil_printf("Ending\n");
	return 0;
}
