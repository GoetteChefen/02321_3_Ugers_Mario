#ifndef IO_INT_H
#define IO_INT_H

/* ------------------------------------------------------------ */
/*					Miscellaneous Declarations					*/
/* ------------------------------------------------------------ */


#include "xparameters.h"
#include "xgpio.h"
#include "xstatus.h"
#include "xtmrctr.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xtime_l.h"

#define BTNS_DEVICE_ID		XPAR_GPIO_0_DEVICE_ID
#define LEDS_DEVICE_ID		XPAR_GPIO_2_DEVICE_ID
#define SWITCH_DEVICE_ID	XPAR_GPIO_3_DEVICE_ID
#define INTC_DEVICE_ID 		XPAR_PS7_SCUGIC_0_DEVICE_ID
#define TMR_DEVICE_ID		XPAR_TMRCTR_0_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID XPAR_FABRIC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR
#define INTC_TMR_INTERRUPT_ID XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR


#define BTN_INT 			XGPIO_IR_CH1_MASK
#define TMR_LOAD			3333333
#define TMR_LOAD_1			100000000


/* ------------------------------------------------------------ */
/*					Procedure Declarations						*/
/* ------------------------------------------------------------ */

u8 btn_flag;
u8 btn_value;

u8 tmr_flag;
u8 framechange;


void initialiseIOandInterupts();

void BTN_Intr_Handler(void *InstancePtr);

int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);

void TMR_Intr_Handler(void *InstancePtr, u8 TmrCtrNumber);

int IntcInitFunction(u16 DeviceId, XTmrCtr *TmrInstancePtr, XGpio *GpioInstancePtr);

void XTmrCtr_ClearInterruptFlag(XTmrCtr * InstancePtr, u8 TmrCtrNumber);

/* ------------------------------------------------------------ */

/************************************************************************/

#endif /* BTN_INT_H */
