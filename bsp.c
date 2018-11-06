/*****************************************************************************
* bsp.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

/**/
#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "xintc.h"
#include "xil_exception.h"
#include "xparameters.h"
#include "xtmrctr.h"
#include "xgpio.h"
#include "xintc.h"
#include "xil_printf.h"
#include "xspi.h"
#include "lcd.h"

/*****************************/

/* Define all variables and Gpio objects here  */

#define GPIO_CHANNEL1 1
#define TIMER_DELAY 2*10000*10000

void debounceInterrupt(); // Write This function

// Create two interrupt controllers XIntc

XIntc per_intc; // Interrupt controller

// Create two static XGpio variables

static XGpio per_encoder;
static XGpio per_btns;
static XGpio per_leds;
static XGpio per_dc;
static XSpi per_spi;
static XTmrCtr per_timer;

volatile int encoder_interrupt = 0;
volatile int mode_interrupt = 0;
// Suggest Creating two int's to use for determining the direction of twist

volatile unsigned int encoderState = 0;

void twistLeft() {
	encoder_interrupt = 1;
	if (AO_Lab2A.volume > 0) {
		AO_Lab2A.volume--;
	}

	//xil_printf("l");
}

void twistRight() {
	encoder_interrupt = 1;
	if (AO_Lab2A.volume < 63) {
		AO_Lab2A.volume++;
	}

	//xil_printf("r");
}

void updateEncoderState(u32 encoderData) {
	u32 button = encoderData & 0b100;

	if (button) {
		encoder_interrupt = 1;
		AO_Lab2A.volume = 0;
	}

	u32 ab = encoderData & 0b11;

	switch(encoderState) {
	case 0:
		if        (ab == 0b01) {
			encoderState = 1;
		} else if (ab == 0b10) {
			encoderState = 4;
		}
		break;
	case 1:
		if        (ab == 0b11) {
			encoderState = 0;
		} else if (ab == 0b00) {
			encoderState = 2;
		}
		break;
	case 2:
		if        (ab == 0b01) {
			encoderState = 1;
		} else if (ab == 0b10) {
			encoderState = 3;
		}
		break;
	case 3:
		if        (ab == 0b11) {
			encoderState = 0;

			twistRight();
		} else if (ab == 0b00) {
			encoderState = 2;
		}
		break;
	case 4:
		if        (ab == 0b00) {
			encoderState = 5;
		} else if (ab == 0b11) {
			encoderState = 0;
		}
		break;
	case 5:
		if        (ab == 0b01) {
			encoderState = 6;
		} else if (ab == 0b10) {
			encoderState = 4;
		}
		break;
	case 6:
		if        (ab == 0b11) {
			encoderState = 0;

			twistLeft();
		} else if (ab == 0b00) {
			encoderState = 5;
		}
		break;
	}
}

/*..........................................................................*/
void BSP_init(void) {
/* Setup LED's, etc */
/* Setup interrupts and reference to interrupt handler function(s)  */

	/*
	 * Initialize the interrupt controller driver so that it's ready to use.
	 * specify the device ID that was generated in xparameters.h
	 *
	 * Initialize GPIO and connect the interrupt controller to the GPIO.
	 *
	 */

	XSpi_Config *spiConfig;	/* Pointer to Configuration data */
	u32 controlReg;
	// 16 leds
	XGpio_Initialize(&per_leds, XPAR_LEDS_DEVICE_ID);

	// encoder
	XGpio_Initialize(&per_encoder, XPAR_ENCODER_DEVICE_ID);

	// buttons
	XGpio_Initialize(&per_btns, XPAR_BTNS_DEVICE_ID);

	// lcd dc
	XGpio_Initialize(&per_dc, XPAR_SPI_DC_DEVICE_ID);

	// interrupt controller
	XIntc_Initialize(&per_intc, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);

	// timer
	XTmrCtr_Initialize(&per_timer, XPAR_AXI_TIMER_0_DEVICE_ID);

	/*
	 * Initialize the SPI driver so that it is  ready to use.
	 * Initialize the SPI driver so that it is  ready to use.
	 */

	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);

	XSpi_CfgInitialize(&per_spi, spiConfig, spiConfig->BaseAddress);

	/*
	 * Reset the SPI device to leave it in a known good state.
	 */
	XSpi_Reset(&per_spi);

	// register encoder handler
	XIntc_Connect(
		&per_intc,
		XPAR_MICROBLAZE_0_AXI_INTC_ENCODER_IP2INTC_IRPT_INTR,
		TwistHandler,
		&per_encoder
	);
	XIntc_Enable(&per_intc, XPAR_MICROBLAZE_0_AXI_INTC_ENCODER_IP2INTC_IRPT_INTR);

	// register button handler
	XIntc_Connect(
		&per_intc,
		XPAR_MICROBLAZE_0_AXI_INTC_BTNS_IP2INTC_IRPT_INTR,
		ButtonHandler,
		&per_encoder
	);
	XIntc_Enable(&per_intc, XPAR_MICROBLAZE_0_AXI_INTC_BTNS_IP2INTC_IRPT_INTR);

//	// register timer handler
	XIntc_Connect(
		&per_intc,
		XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR,
		TimerHandler,
		&per_timer
	);
	XIntc_Enable(&per_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);

	XIntc_Start(&per_intc, XIN_REAL_MODE);

	// set up the encoder
	XGpio_InterruptEnable(&per_encoder, 1);
	XGpio_InterruptGlobalEnable(&per_encoder);
	XGpio_SetDataDirection(&per_encoder, 1, 0xFFFFFFFF);

	// set up the buttons
	XGpio_InterruptEnable(&per_btns, 1);
	XGpio_InterruptGlobalEnable(&per_btns);
	XGpio_SetDataDirection(&per_btns, 1, 0xFFFFFFFF);

	// set up the lcd dc
	XGpio_SetDataDirection(&per_dc, 1, 0x0);

	// set up the timer
	XTmrCtr_SetOptions(
		&per_timer,
		0,
		XTC_INT_MODE_OPTION
	);

	// timer time
	XTmrCtr_SetResetValue(&per_timer, 0, 0xFFFFFFFF-TIMER_DELAY);

	/*
	 * Setup the control register to enable master mode
	 */
	controlReg = XSpi_GetControlReg(&per_spi);
	XSpi_SetControlReg(&per_spi,
			(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
			(~XSP_CR_TRANS_INHIBIT_MASK));

	// Select 1st slave device
	XSpi_SetSlaveSelectReg(&per_spi, ~0x01);

	xil_printf("Starting Init\n");
	initLCD();

	xil_printf("Clearing Screen\n");
	clrScr();

	// connect interrupt controller to microblaze
	microblaze_register_handler(
			(XInterruptHandler)XIntc_DeviceInterruptHandler,
		(void*)XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID
	);

	microblaze_enable_interrupts();
}
/*..........................................................................*/
void QF_onStartup(void) {                 /* entered with interrupts locked */

/* Enable interrupts */
	xil_printf("\n\rQF_onStartup\n"); // Comment out once you are in your complete program
	fillBackground(0,0,239,319);


	// Press Knob
	// Enable interrupt controller
	// Start interupt controller
	// register handler with Microblaze
	// Global enable of interrupt
	// Enable interrupt on the GPIO

	// Twist Knob

	// General
	// Initialize Exceptions
	// Press Knob
	// Register Exception
	// Twist Knob
	// Register Exception
	// General
	// Enable Exception

	// Variables for reading Microblaze registers to debug your interrupts.
//	{
//		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
//		u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
//		u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
//		u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
//		u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
//		u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
//		u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
//		u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
//		u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
//		u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
//		u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
//		u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
//		u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003; // & 0xMASK
//		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000; // & 0xMASK
//	}
}


void QF_onIdle(void) {        /* entered with interrupts locked */

    QF_INT_UNLOCK();                       /* unlock interrupts */

    {
    	// Write code to increment your interrupt counter here.
    	// QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN); is used to post an event to your FSM
    	//xil_printf("waiting\n");

    	//Lab2A* me = &AO_Lab2A;

    	if ((AO_Lab2A.mode_drawn != AO_Lab2A.mode)|| mode_interrupt) {
    		mode_interrupt = 0;
    		AO_Lab2A.mode_drawn = AO_Lab2A.mode;
    		drawMode(AO_Lab2A.mode);

    	} else if ((AO_Lab2A.volume_drawn != AO_Lab2A.volume)  || encoder_interrupt) {
    		encoder_interrupt = 0;
    		int temp_volume = AO_Lab2A.volume_drawn;
    		AO_Lab2A.volume_drawn = AO_Lab2A.volume;
    		drawVolume(temp_volume, AO_Lab2A.volume, !AO_Lab2A.volume_on_screen);
    		AO_Lab2A.volume_on_screen = 1;
    	}
// 			Useful for Debugging, and understanding your Microblaze registers.
//    		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
//    	    u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
//    	    u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
//
//    	    u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
//    	    u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
//    	    u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
//    	    u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
//    	    u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
//    	    u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
//    	    u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
//    	    u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
//
//    	    // Expect to see 0x00000001
//    	    u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
//    	    // Expect to see 0x00000001
//    	    u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003;
//
//    	    // Expect to see 0x80000000 in GIER
//    		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000;


    }
}

/* Do not touch Q_onAssert */
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    (void)file;                                   /* avoid compiler warning */
    (void)line;                                   /* avoid compiler warning */
    QF_INT_LOCK();
    for (;;) {
    }
}

/* Interrupt handler functions here.  Do not forget to include them in lab2a.h!
To post an event from an ISR, use this template:
QActive_postISR((QActive *)&AO_Lab2A, SIGNALHERE);
Where the Signals are defined in lab2a.h  */

/******************************************************************************
*
* This is the interrupt handler routine for the GPIO for this example.
*
******************************************************************************/
void ButtonHandler(void *CallbackRef) {
	// Increment A counter
	u32 data = XGpio_DiscreteRead(&per_btns, 1);
	XGpio_DiscreteWrite(&per_leds, 1, data);
	if(data&0b10000){
		AO_Lab2A.mode = 5;
	}
	else if(data&0b01000){AO_Lab2A.mode = 4;}
	else if(data&0b00100){AO_Lab2A.mode = 3;}
	else if(data&0b00010){AO_Lab2A.mode = 2;}
	else if(data&0b00001){AO_Lab2A.mode = 1;}
	mode_interrupt = 1;
	// start the timer
	XTmrCtr_Start(&per_timer, 0);

	// mark interrupt as handled
	XGpio_InterruptClear(&per_btns, 0xFFFFFFFF);
}

// Encoder
void TwistHandler(void *CallbackRef) {
	u32 data = XGpio_DiscreteRead(&per_encoder, 1);
	XGpio_DiscreteWrite(&per_leds, 1, data);

	updateEncoderState(data);
	// start the timer
	XTmrCtr_Start(&per_timer, 0);

	// mark interrupt as handled
	XGpio_InterruptClear(&per_encoder, 0xFFFFFFFF);
}

void TimerHandler(void * CallbackRef) {
	// handler code
	AO_Lab2A.volume_on_screen = 0;
	clearVolume();

	AO_Lab2A.mode_on_screen = 0;
	clearMode();

	// acknowledge that interrupt handled
	u32 controlReg = XTimerCtr_ReadReg(per_timer.BaseAddress, 0, XTC_TCSR_OFFSET);
	XTmrCtr_WriteReg(
		per_timer.BaseAddress,
		0,
		XTC_TCSR_OFFSET,
		controlReg | XTC_CSR_INT_OCCURED_MASK
	);
}

void debounceTwistInterrupt(){
	// Read both lines here? What is twist[0] and twist[1]?
	// How can you use reading from the two GPIO twist input pins to figure out which way the twist is going?
}

void debounceInterrupt() {
	QActive_postISR((QActive *)&AO_Lab2A, ENCODER_CLICK);
	// XGpio_InterruptClear(&sw_Gpio, GPIO_CHANNEL1); // (Example, need to fill in your own parameters
}
