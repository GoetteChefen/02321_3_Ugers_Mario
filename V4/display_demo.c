/************************************************************************/
/*																		*/
/*	display_demo.c	--	ZYBO Display demonstration 						*/
/*																		*/
/************************************************************************/
/*	Author: Sam Bobrowicz												*/
/*	Copyright 2016, Digilent Inc.										*/
/************************************************************************/
/*  Module Description: 												*/
/*																		*/
/*		This file contains code for running a demonstration of the		*/
/*		HDMI output capabilities on the ZYBO. It is a good	            */
/*		example of how to properly use the display_ctrl drivers.	    */
/*																		*/
/************************************************************************/
/*  Revision History:													*/
/* 																		*/
/*		2/5/2016(SamB): Created											*/
/*																		*/
/************************************************************************/

/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include "display_demo.h"
#include "display_ctrl/display_ctrl.h"
#include <stdio.h>
#include "xuartps.h"
#include "math.h"
#include <ctype.h>
#include <stdlib.h>
#include "xil_types.h"
#include "xil_cache.h"
#include "timer_ps/timer_ps.h"
#include "xparameters.h"
#include "interupts/interuptIO.h"

/*
 * XPAR redefines
 */
#define DYNCLK_BASEADDR XPAR_AXI_DYNCLK_0_BASEADDR
#define VGA_VDMA_ID XPAR_AXIVDMA_0_DEVICE_ID
#define DISP_VTC_ID XPAR_VTC_0_DEVICE_ID
#define VID_VTC_IRPT_ID XPS_FPGA3_INT_ID
#define VID_GPIO_IRPT_ID XPS_FPGA4_INT_ID
#define SCU_TIMER_ID XPAR_SCUTIMER_DEVICE_ID
#define UART_BASEADDR XPAR_PS7_UART_1_BASEADDR

/* ------------------------------------------------------------ */
/*				Global Variables								*/
/* ------------------------------------------------------------ */

/*
 * Display Driver structs
 */
DisplayCtrl dispCtrl;
XAxiVdma vdma;

/*
 * Framebuffers for video data
 */
u8  frameBuf[DISPLAY_NUM_FRAMES][DEMO_MAX_FRAME] __attribute__((aligned(0x20)));
u8 *pFrames[DISPLAY_NUM_FRAMES]; //array of pointers to the frame buffers

/* ------------------------------------------------------------ */
/*				Procedure Definitions							*/
/* ------------------------------------------------------------ */




void printsinglecolor(int col, u8 *frame, u32 width, u32 height, u32 stride);
void PrintSquare_V2(u8 *frame, u32 width, u32 height, u32 stride, u8 *sprite, u32 xcoi, u32 ycoi, u32 spriteWidth, u32 spriteHeight);

#define testArrayHeight 5
#define testArrayWidth 20
u8 blackTestArray [testArrayWidth*testArrayHeight];
u8 whiteTestArray [testArrayWidth*testArrayHeight];



int main(void)
{
	DemoInitialize();

	initialiseIOandInterupts();

	btn_flag = 0;

	printsinglecolor(0, pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE);

	int vertical = 		100;
	int horizontal = 	100;

	PrintSquare_V2(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE, blackTestArray, horizontal, vertical, testArrayWidth, testArrayHeight);

	while(1){

		if(btn_flag == 1){

			PrintSquare_V2(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE, whiteTestArray, horizontal, vertical, testArrayWidth, testArrayHeight);

			switch (btn_value){

			case 1:
				vertical += 10;
				break;

			case 2:
				vertical -= 10;
				break;

			case 4:
				horizontal += 10;
				break;

			case 8:
				horizontal -= 10;
				break;
			}


			PrintSquare_V2(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE, blackTestArray, horizontal, vertical, testArrayWidth, testArrayHeight);

			btn_flag = 0;
		}
	}

	DemoRun();

	return 0;
}

void DemoInitialize()
{
    for (int i = 0; i < testArrayWidth*testArrayHeight; ++i) {
       blackTestArray[i] = 0;
    }

    for (int i = 0; i < testArrayWidth*testArrayHeight; ++i) {
       whiteTestArray[i] = 255;
    }

	int Status;
	XAxiVdma_Config *vdmaConfig;
	int i;

	/*
	 * Initialize an array of pointers to the 3 frame buffers
	 */
	for (i = 0; i < DISPLAY_NUM_FRAMES; i++)
	{
		pFrames[i] = frameBuf[i];
	}

	/*
	 * Initialize a timer used for a simple delay
	 */
	TimerInitialize(SCU_TIMER_ID);

	/*
	 * Initialize VDMA driver
	 */
	vdmaConfig = XAxiVdma_LookupConfig(VGA_VDMA_ID);
	if (!vdmaConfig)
	{
		xil_printf("No video DMA found for ID %d\r\n", VGA_VDMA_ID);
		return;
	}
	Status = XAxiVdma_CfgInitialize(&vdma, vdmaConfig, vdmaConfig->BaseAddress);
	if (Status != XST_SUCCESS)
	{
		xil_printf("VDMA Configuration Initialization failed %d\r\n", Status);
		return;
	}

	/*
	 * Initialize the Display controller and start it
	 */
	Status = DisplayInitialize(&dispCtrl, &vdma, DISP_VTC_ID, DYNCLK_BASEADDR, pFrames, DEMO_STRIDE);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Display Ctrl initialization failed during demo initialization%d\r\n", Status);
		return;
	}
	Status = DisplayStart(&dispCtrl);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Couldn't start display during demo initialization%d\r\n", Status);
		return;
	}

	DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_1);

	return;
}

void PrintSinglePixel(u8 *frame, u32 width, u32 height, u32 stride) {
	u32 xcoi, ycoi;

	xil_printf("\x1B[H"); //Set cursor to top left of terminal
	xil_printf("\x1B[2J"); //Clear terminal

	xil_printf("Type x coordinate\n\r");
	scanf("%lu", &xcoi);
	xil_printf("Type y coordinate\n\r");
	scanf("%lu", &ycoi);

     //Green background
	if(xcoi < height && ycoi < width) {
    	frame[(xcoi*4) + (ycoi*stride)] = 170;
    	frame[(xcoi*4) + (ycoi*stride) + 1] = 170;
    	frame[(xcoi*4) + (ycoi*stride) + 2] = 170;

    	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
	}

}

void PrintSquare(u8 *frame, u32 width, u32 height, u32 stride) {
	u32 xcoi, ycoi;
	int sideLength = 40;
	u32 lineStart = 0;

	xil_printf("\x1B[H"); //Set cursor to top left of terminal
	xil_printf("\x1B[2J"); //Clear terminal

	xil_printf("Type x coordinate\n\r");
	scanf("%lu", &xcoi);
	xil_printf("Type y coordinate\n\r");
	scanf("%lu", &ycoi);


	if(xcoi < height && ycoi < width) {
		for(int y = 0; y < sideLength; y++){
			for(int x = 0; x < sideLength*4; x+=4) {
				frame[(xcoi*4) + (ycoi*stride) + x + lineStart] = 170;
				frame[(xcoi*4) + (ycoi*stride) + 1 + x + lineStart] = 170;
				frame[(xcoi*4) + (ycoi*stride) + 2 + x + lineStart] = 170;
			}
			lineStart += stride;
		}

    	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
	}

}


void PrintSquare_V2(u8 *frame, u32 width, u32 height, u32 stride, u8 *sprite, u32 xcoi, u32 ycoi, u32 spriteWidth, u32 spriteHeight) {

	u32 lineStart = 0;

	if(xcoi < height && ycoi < width) {

		for(int y = 0; y < spriteHeight; y++){

			for(int x = 0; x < spriteWidth; x+=4) {

				frame[(xcoi*4) + (ycoi*stride) + 	 x + lineStart] = sprite[x];
				frame[(xcoi*4) + (ycoi*stride) + 1 + x + lineStart] = sprite[x + 1];
				frame[(xcoi*4) + (ycoi*stride) + 2 + x + lineStart] = sprite[x + 2];
				frame[(xcoi*4) + (ycoi*stride) + 3 + x + lineStart] = sprite[x + 3];
			}
			lineStart += stride;
		}

    	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
	}

}


void PrintSquare_V3(u8 *frame, u32 width, u32 height, u32 stride, u8 *sprite, u32 xcoi, u32 ycoi, u32 spriteWidth, u32 spriteHeight) {

	u32 lineStart = 0;

	u32 x1 = 0;

	if(xcoi < height && ycoi < width) {

		for(int y = 0; y < spriteHeight; y++){

			for(int x = 0; x < spriteWidth; x+=4) {

				frame[(xcoi*4) + (ycoi*stride) + 	 x + lineStart] = sprite[x1];
				frame[(xcoi*4) + (ycoi*stride) + 1 + x + lineStart] = sprite[x1 + 1];
				frame[(xcoi*4) + (ycoi*stride) + 2 + x + lineStart] = sprite[x1 + 2];
				//frame[(xcoi*4) + (ycoi*stride) + 3 + x + lineStart] = sprite[x + 3];

				x1 += 3;
			}
			lineStart += stride;
		}

    	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
	}

}

void printsinglecolor(int col, u8 *frame, u32 width, u32 height, u32 stride){
    u32 xcoi, ycoi;
    u32 lineStart = 0;

    switch (col) {
        case 0:

            for(ycoi = 0; ycoi < height; ycoi++)
            {
                for(xcoi = 0; xcoi < (width * 4); xcoi+=4)
                {
                    frame[xcoi + lineStart] =  255;   //Blue
                    frame[xcoi + lineStart + 1] = 255;  //Green
                    frame[xcoi + lineStart + 2] = 255;  //Red
                }
                lineStart += stride;
            }

            Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);



            break;
        case 1:

            for(ycoi = 0; ycoi < height; ycoi++)
            {
                for(xcoi = 0; xcoi < (width * 4); xcoi+=4)
                {
                    frame[xcoi + lineStart] =  0;   //Blue
                    frame[xcoi + lineStart + 1] = 0;  //Green
                    frame[xcoi + lineStart + 2] = 255;  //Red
                }
                lineStart += stride;
            }
            Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);

            break;
        case 2:

            for(ycoi = 0; ycoi < height; ycoi++)
            {
                for(xcoi = 0; xcoi < (width * 4); xcoi+=4)
                {
                    frame[xcoi + lineStart] =  255;   //Blue
                    frame[xcoi + lineStart + 1] = 0;  //Green
                    frame[xcoi + lineStart + 2] = 0;  //Red
                }
                lineStart += stride;
            }
            Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);

            break;

        default:
            break;

    }
    return;
}

void DemoRun()
{
	int nextFrame = 0;
	char userInput = 0;

	/* Flush UART FIFO */
	while (XUartPs_IsReceiveData(UART_BASEADDR))
	{
		XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
	}

	while (userInput != 'q')
	{
		DemoPrintMenu();

		/* Wait for data on UART */
		while (!XUartPs_IsReceiveData(UART_BASEADDR))
		{}

		/* Store the first character in the UART receive FIFO and echo it */
		if (XUartPs_IsReceiveData(UART_BASEADDR))
		{
			userInput = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
			xil_printf("%c", userInput);
		}

		switch (userInput)
		{
		case '1':
			DemoChangeRes();
			break;
		case '2':
			nextFrame = dispCtrl.curFrame + 1;
			if (nextFrame >= DISPLAY_NUM_FRAMES)
			{
				nextFrame = 0;
			}
			DisplayChangeFrame(&dispCtrl, nextFrame);
			break;
		case '3':
			DemoPrintTest(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE, DEMO_PATTERN_0);
			break;
		case '4':
			DemoPrintTest(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE, DEMO_PATTERN_1);
			break;
		case '5':
			DemoInvertFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride);
			break;
		case '6':
			nextFrame = dispCtrl.curFrame + 1;
			if (nextFrame >= DISPLAY_NUM_FRAMES)
			{
				nextFrame = 0;
			}
			DemoInvertFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride);
			DisplayChangeFrame(&dispCtrl, nextFrame);
			break;

		case '7': //green
            printsinglecolor(0, pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE);
			break;

        case '8': //red
            printsinglecolor(1, pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE);
            break;

        case '9': //blue
            printsinglecolor(2, pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE);
            break;

        case 'a': //Print single pixel
        	PrintSquare(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE);
        	break;

        case 'b': //Print sprite
        	PrintSquare_V3(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE, mario, 0, 0, 512*4, 480);
            break;

		case 'q':
			break;
		default :
			xil_printf("\n\rInvalid Selection");
			TimerDelay(500000);
		}
	}

	return;
}

void DemoPrintMenu()
{
	xil_printf("\x1B[H"); //Set cursor to top left of terminal
	xil_printf("\x1B[2J"); //Clear terminal
	xil_printf("**************************************************\n\r");
	xil_printf("*               ZYBO Display Demo                *\n\r");
	xil_printf("**************************************************\n\r");
	xil_printf("*Display Resolution: %28s*\n\r", dispCtrl.vMode.label);
	printf("*Display Pixel Clock Freq. (MHz): %15.3f*\n\r", dispCtrl.pxlFreq);
	xil_printf("*Display Frame Index: %27d*\n\r", dispCtrl.curFrame);
	xil_printf("**************************************************\n\r");
	xil_printf("\n\r");
	xil_printf("1 - Change Display Resolution\n\r");
	xil_printf("2 - Change Display Framebuffer Index\n\r");
	xil_printf("3 - Print Blended Test Pattern to Display Framebuffer\n\r");
	xil_printf("4 - Print Color Bar Test Pattern to Display Framebuffer\n\r");
	xil_printf("5 - Invert Current Frame colors\n\r");
	xil_printf("6 - Invert Current Frame colors seamlessly\n\r");
	xil_printf("7 - Print single color to screen (green)\n\r");
    xil_printf("8 - Print single color to screen (red)\n\r");
    xil_printf("9 - Print single color to screen (blue)\n\r");
	xil_printf("q - Quit\n\r");
	xil_printf("\n\r");
	xil_printf("\n\r");
	xil_printf("Enter a selection:");
}

void DemoChangeRes()
{
	int fResSet = 0;
	int status;
	char userInput = 0;

	/* Flush UART FIFO */
	while (XUartPs_IsReceiveData(UART_BASEADDR))
	{
		XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
	}

	while (!fResSet)
	{
		DemoCRMenu();

		/* Wait for data on UART */
		while (!XUartPs_IsReceiveData(UART_BASEADDR))
		{}

		/* Store the first character in the UART recieve FIFO and echo it */
		userInput = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
		xil_printf("%c", userInput);
		status = XST_SUCCESS;
		switch (userInput)
		{
		case '1':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_640x480);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '2':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_800x600);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '3':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1280x720);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '4':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1280x1024);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '5':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1920x1080);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case 'q':
			fResSet = 1;
			break;
		default :
			xil_printf("\n\rInvalid Selection");
			TimerDelay(500000);
		}
		if (status == XST_DMA_ERROR)
		{
			xil_printf("\n\rWARNING: AXI VDMA Error detected and cleared\n\r");
		}
	}
}

void DemoCRMenu()
{
	xil_printf("\x1B[H"); //Set cursor to top left of terminal
	xil_printf("\x1B[2J"); //Clear terminal
	xil_printf("**************************************************\n\r");
	xil_printf("*               ZYBO Display Demo                *\n\r");
	xil_printf("**************************************************\n\r");
	xil_printf("*Current Resolution: %28s*\n\r", dispCtrl.vMode.label);
	printf("*Pixel Clock Freq. (MHz): %23.3f*\n\r", dispCtrl.pxlFreq);
	xil_printf("**************************************************\n\r");
	xil_printf("\n\r");
	xil_printf("1 - %s\n\r", VMODE_640x480.label);
	xil_printf("2 - %s\n\r", VMODE_800x600.label);
	xil_printf("3 - %s\n\r", VMODE_1280x720.label);
	xil_printf("4 - %s\n\r", VMODE_1280x1024.label);
	xil_printf("5 - %s\n\r", VMODE_1920x1080.label);
	xil_printf("q - Quit (don't change resolution)\n\r");
	xil_printf("\n\r");
	xil_printf("Select a new resolution:");
}

void DemoInvertFrame(u8 *srcFrame, u8 *destFrame, u32 width, u32 height, u32 stride)
{
	u32 xcoi, ycoi;
	u32 lineStart = 0;
	for(ycoi = 0; ycoi < height; ycoi++)
	{
		for(xcoi = 0; xcoi < (width * 4); xcoi+=4)
		{
			destFrame[xcoi + lineStart] = ~srcFrame[xcoi + lineStart];         //Red
			destFrame[xcoi + lineStart + 1] = ~srcFrame[xcoi + lineStart + 1]; //Blue
			destFrame[xcoi + lineStart + 2] = ~srcFrame[xcoi + lineStart + 2]; //Green
		}
		lineStart += stride;
	}
	/*
	 * Flush the framebuffer memory range to ensure changes are written to the
	 * actual memory, and therefore accessible by the VDMA.
	 */
	Xil_DCacheFlushRange((unsigned int) destFrame, DEMO_MAX_FRAME);
}

void DemoPrintTest(u8 *frame, u32 width, u32 height, u32 stride, int pattern)
{
	u32 xcoi, ycoi;
	u32 iPixelAddr;
	u8 wRed, wBlue, wGreen;
	u32 wCurrentInt;
	double fRed, fBlue, fGreen, fColor;
	u32 xLeft, xMid, xRight, xInt;
	u32 yMid, yInt;
	double xInc, yInc;


	switch (pattern)
	{
	case DEMO_PATTERN_0:

		xInt = width / 4; //Four intervals, each with width/4 pixels
		xLeft = xInt * 3;
		xMid = xInt * 2 * 3;
		xRight = xInt * 3 * 3;
		xInc = 256.0 / ((double) xInt); //256 color intensities are cycled through per interval (overflow must be caught when color=256.0)

		yInt = height / 2; //Two intervals, each with width/2 lines
		yMid = yInt;
		yInc = 256.0 / ((double) yInt); //256 color intensities are cycled through per interval (overflow must be caught when color=256.0)

		fBlue = 0.0;
		fRed = 256.0;
		for(xcoi = 0; xcoi < (width*4); xcoi+=4)
		{
			/*
			 * Convert color intensities to integers < 256, and trim values >=256
			 */
			wRed = (fRed >= 256.0) ? 255 : ((u8) fRed);
			wBlue = (fBlue >= 256.0) ? 255 : ((u8) fBlue);
			iPixelAddr = xcoi;
			fGreen = 0.0;
			for(ycoi = 0; ycoi < height; ycoi++)
			{

				wGreen = (fGreen >= 256.0) ? 255 : ((u8) fGreen);
				frame[iPixelAddr] = wRed;
				frame[iPixelAddr + 1] = wBlue;
				frame[iPixelAddr + 2] = wGreen;
				if (ycoi < yMid)
				{
					fGreen += yInc;
				}
				else
				{
					fGreen -= yInc;
				}

				/*
				 * This pattern is printed one vertical line at a time, so the address must be incremented
				 * by the stride instead of just 1.
				 */
				iPixelAddr += stride;
			}

			if (xcoi < xLeft)
			{
				fBlue = 0.0;
				fRed -= xInc;
			}
			else if (xcoi < xMid)
			{
				fBlue += xInc;
				fRed += xInc;
			}
			else if (xcoi < xRight)
			{
				fBlue -= xInc;
				fRed -= xInc;
			}
			else
			{
				fBlue += xInc;
				fRed = 0;
			}
		}
		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
		break;
	case DEMO_PATTERN_1:

		xInt = width / 7; //Seven intervals, each with width/7 pixels
		xInc = 256.0 / ((double) xInt); //256 color intensities per interval. Notice that overflow is handled for this pattern.

		fColor = 0.0;
		wCurrentInt = 1;
		for(xcoi = 0; xcoi < (width*4); xcoi+=4)
		{

			/*
			 * Just draw white in the last partial interval (when width is not divisible by 7)
			 */
			if (wCurrentInt > 7)
			{
				wRed = 255;
				wBlue = 255;
				wGreen = 255;
			}
			else
			{
				if (wCurrentInt & 0b001)
					wRed = (u8) fColor;
				else
					wRed = 0;

				if (wCurrentInt & 0b010)
					wBlue = (u8) fColor;
				else
					wBlue = 0;

				if (wCurrentInt & 0b100)
					wGreen = (u8) fColor;
				else
					wGreen = 0;
			}

			iPixelAddr = xcoi;

			for(ycoi = 0; ycoi < height; ycoi++)
			{
				frame[iPixelAddr] = wRed;
				frame[iPixelAddr + 1] = wBlue;
				frame[iPixelAddr + 2] = wGreen;
				/*
				 * This pattern is printed one vertical line at a time, so the address must be incremented
				 * by the stride instead of just 1.
				 */
				iPixelAddr += stride;
			}

			fColor += xInc;
			if (fColor >= 256.0)
			{
				fColor = 0.0;
				wCurrentInt++;
			}
		}
		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
		break;
	default :
		xil_printf("Error: invalid pattern passed to DemoPrintTest");
	}
}


