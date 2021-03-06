//--------------------------------------------------------------------------//
//----------------------- COPIED FROM AVR32 EXAMPLE ------------------------//
//--------------------------------------------------------------------------//

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief FreeRTOS Serial Port management example for AVR32 UC3.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 *****************************************************************************/

/* Copyright (c) 2007, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
  BASIC INTERRUPT DRIVEN SERIAL PORT DRIVER FOR USART.
*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* Demo application includes. */
#include "serial.h"

/* bsp includes. */
#include "support.h"
#include "spr_defs.h"
#include "uart.h"
#include "interrupts.h"

/*-----------------------------------------------------------*/

/* Constants to setup and access the USART. */
#define serINVALID_COMPORT_HANDLER        ( ( xComPortHandle ) 0 )
#define serINVALID_QUEUE                  ( ( xQueueHandle ) 0 )
#define serHANDLE                         ( ( xComPortHandle ) 1 )
#define serNO_BLOCK                       ( ( portTickType ) 0 )

/*-----------------------------------------------------------*/

/* Queues used to hold received characters, and characters waiting to be
transmitted. */
static xQueueHandle xRxedChars;
static xQueueHandle xCharsForTx;

/*-----------------------------------------------------------*/

/* Forward declaration. */
static void vprvSerialCreateQueues( unsigned portBASE_TYPE uxQueueLength,
									xQueueHandle *pxRxedChars,
									xQueueHandle *pxCharsForTx );

/*-----------------------------------------------------------*/
static void vUSART_ISR( void *arg )
{
	/* Now we can declare the local variables. */
	arg = arg;
	signed portCHAR cChar;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	unsigned portLONG ulStatus;
	portBASE_TYPE retstatus;

	/* What caused the interrupt? */
	ulStatus = uart_get_iir(0);
	
	// TX RADY INTERRUPT
	if (ulStatus & UART_IIR_THRI)
	{
		/* The interrupt was caused by the THR becoming empty.  Are there any
		more characters to transmit?
		Because FreeRTOS is not supposed to run with nested interrupts, put all OS
		calls in a critical section . */
			
		/* entering, exiting ciritical section around xQueueReceiveFromISR is not 
		required. OpenRISC automaticaly disable interrupt when expection occurs */
		retstatus = xQueueReceiveFromISR( xCharsForTx, &cChar, &xHigherPriorityTaskWoken );

		if (retstatus == pdTRUE)
		{
			/* A character was retrieved from the queue so can be sent to the
			 THR now. */
			uart_putc_noblock(0, cChar);
		}
		else
		{
			/* Queue empty, nothing to send so turn off the Tx interrupt. */
			uart_txint_disable(0);
		}
	}

	// RX RADY INTERRUPT
	if (ulStatus & UART_IIR_RDI)
	{
		/* The interrupt was caused by the receiver getting data. */
		cChar = uart_getc_noblock(0); 

		/* Because FreeRTOS is not supposed to run with nested interrupts, put all OS
		calls in a critical section . but in case of OpenRISC, it is not required. Tick
		, External interrupt are automaticaly disabled. */
		xQueueSendFromISR(xRxedChars, &cChar, &xHigherPriorityTaskWoken);
	}

	/* The return value will be used by portEXIT_SWITCHING_ISR() to know if it
	should perform a vTaskSwitchContext(). */
	// return ( xHigherPriorityTaskWoken );
}
/*-----------------------------------------------------------*/


/*
 * Init the serial port for the Minimal implementation.
 */
xComPortHandle xSerialPortInitMinimal( unsigned portLONG ulWantedBaud, unsigned portBASE_TYPE uxQueueLength )
{
	xComPortHandle xReturn = serHANDLE;

	/* Create the rx and tx queues. */
	vprvSerialCreateQueues( uxQueueLength, &xRxedChars, &xCharsForTx );

	/* Configure USART. */
	if( ( xRxedChars != serINVALID_QUEUE ) &&
		( xCharsForTx != serINVALID_QUEUE ) &&
		( ulWantedBaud != ( unsigned portLONG ) 0 ) )
	{
		portENTER_CRITICAL();
		{
			// register interrupt handler
			int_add(UART0_IRQ, vUSART_ISR, 0x0);
		}
		portEXIT_CRITICAL();
	}
	else
	{
		xReturn = serINVALID_COMPORT_HANDLER;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE xSerialGetChar( xComPortHandle pxPort, signed portCHAR *pcRxedChar, portTickType xBlockTime )
{
	/* The port handle is not required as this driver only supports UART0. */
	( void ) pxPort;

	/* Get the next character from the buffer.  Return false if no characters
	are available, or arrive before xBlockTime expires. */
	if( xQueueReceive( xRxedChars, pcRxedChar, xBlockTime ) )
	{
		return pdTRUE;
	}
	else
	{
		return pdFALSE;
	}
}
/*-----------------------------------------------------------*/

void vSerialPutString( xComPortHandle pxPort, const signed portCHAR * const pcString, unsigned portSHORT usStringLength )
{
	usStringLength = usStringLength;
	signed portCHAR *pxNext;

	/* NOTE: This implementation does not handle the queue being full as no
	block time is used! */

	/* The port handle is not required as this driver only supports UART0. */
	( void ) pxPort;

	/* Send each character in the string, one at a time. */
	pxNext = ( signed portCHAR * ) pcString;
	while( *pxNext )
	{
		xSerialPutChar( pxPort, *pxNext, serNO_BLOCK );
		pxNext++;
	}
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE xSerialPutChar( xComPortHandle pxPort, signed portCHAR cOutChar, portTickType xBlockTime )
{
	/* The port handle is not required as this driver only supports UART0. */
	( void ) pxPort;

	/* Place the character in the queue of characters to be transmitted. */
	if( xQueueSend( xCharsForTx, &cOutChar, xBlockTime ) != pdPASS )
	{
		return pdFAIL;
	}

	/* Turn on the Tx interrupt so the ISR will remove the character from the
	queue and send it.   This does not need to be in a critical section as
	if the interrupt has already removed the character the next interrupt
	will simply turn off the Tx interrupt again. */
	uart_txint_enable(0);

	return pdPASS;
}
/*-----------------------------------------------------------*/

void vSerialClose( xComPortHandle xPort )
{
  /* Not supported as not required by the demo application. */
  xPort = xPort;				// prevent compiler warning
}
/*-----------------------------------------------------------*/

/*###########################################################*/

/*
 * Create the rx and tx queues.
 */
static void vprvSerialCreateQueues(  unsigned portBASE_TYPE uxQueueLength, xQueueHandle *pxRxedChars, xQueueHandle *pxCharsForTx )
{
	/* Create the queues used to hold Rx and Tx characters. */
	xRxedChars = xQueueCreate( uxQueueLength, ( unsigned portBASE_TYPE ) sizeof( signed portCHAR ) );
	xCharsForTx = xQueueCreate( uxQueueLength + 1, ( unsigned portBASE_TYPE ) sizeof( signed portCHAR ) );

	/* Pass back a reference to the queues so the serial API file can
	post/receive characters. */
	*pxRxedChars = xRxedChars;
	*pxCharsForTx = xCharsForTx;
}
/*-----------------------------------------------------------*/
