/*
    FreeRTOS V6.1.1 - Copyright (C) 2011 Real Time Engineers Ltd.

    ***************************************************************************
    *                                                                         *
    * If you are:                                                             *
    *                                                                         *
    *    + New to FreeRTOS,                                                   *
    *    + Wanting to learn FreeRTOS or multitasking in general quickly       *
    *    + Looking for basic training,                                        *
    *    + Wanting to improve your FreeRTOS skills and productivity           *
    *                                                                         *
    * then take a look at the FreeRTOS books - available as PDF or paperback  *
    *                                                                         *
    *        "Using the FreeRTOS Real Time Kernel - a Practical Guide"        *
    *                  http://www.FreeRTOS.org/Documentation                  *
    *                                                                         *
    * A pdf reference manual is also available.  Both are usually delivered   *
    * to your inbox within 20 minutes to two hours when purchased between 8am *
    * and 8pm GMT (although please allow up to 24 hours in case of            *
    * exceptional circumstances).  Thank you for your support!                *
    *                                                                         *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    ***NOTE*** The exception to the GPL is included to allow you to distribute
    a combined work that includes FreeRTOS without being obliged to provide the
    source code for proprietary components outside of the FreeRTOS kernel.
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public 
    License and the FreeRTOS license exception along with FreeRTOS; if not it 
    can be viewed here: http://www.freertos.org/a00114.html and also obtained 
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

#include "FreeRTOS.h"

/*-----------------------------------------------------------*/

/* Called by the startup code to initialise the run time system. */
unsigned portCHAR __low_level_init(void);

/*-----------------------------------------------------------*/

unsigned portCHAR __low_level_init(void)
{
unsigned portCHAR resetflag = RESF;
unsigned portCHAR psval = 0;

	/* Setup provided by NEC. */

	/* Disable global interrupts to ensure no interrupts occur during system
	setup. */
	portDISABLE_INTERRUPTS();

	PRCMD = 0x00;
	OCDM = 0x00;
	VSWC = 0x12;
	VSWC = 18;

	/* Set main system clock */
	OSTS = 0x06;
	psval = 0x80;
	PRCMD = psval;
	PCC = psval;
	while (!OSTC)
	{
		;
	}

	PLLS = 0x03;
	PLLON = 1;
	while (LOCKR)
	{
		;
	}

	psval = 0x01;
	PRCMD = psval;
	MCM = psval;
	SELPLL = 1;

	/* Set fCPU */
	psval = PCC | 0x00;
	PRCMD = psval;
	PCC = psval;
	RCM = 0x83;

	/* Set fXP1 */
	SELCNT4 = 0x00;

	/* Set fBRG */
	PRSM0 = 0x00;

	/* Stand-by setting */
	psval = 0x00;
	PRCMD = psval;
	PSC = psval;

	/* WDT2 setting */
	WDTM2 = 0x1F;

	/* PCL setting */
	PCLM = 0x00;

	/* disable dma0 - dma3 */
	E00 = 0;	
	E11 = 0;
	E22 = 0;
	E33 = 0;	

	return pdTRUE;
}
/*-----------------------------------------------------------*/
