/**
 * dvruntime Generated Driver Source File 
 *
 * @file DVRunTime.c
 * 
 * @ingroup dvruntime
 * 
 * @brief This file contains the driver implementation for the dvruntime driver module.
 * 
 * @version dvruntime Package Version 2.0.0.
 *
 * @version dvruntime Driver Version 2.0.0.
*/
/*
© [2025] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/ 


/**
  Section: Included Files
*/

#include "../../DVRT/DVRunTime.h"
#include "../../uart/uart1.h"
#include "../../timer/tmr0.h"

/**
  Section: Macro Declarations
*/

extern const uart_drv_interface_t *UART_DV = &UART1;
static const struct TIMER_INTERFACE *TIMER_DVRT = &Timer0;

static DVRT_error_t error;

/**
  Section: Driver Interface
*/
void DVRT_UART_RX_CallBack(void);
void DVRT_HandleCommand(void);
void DVRT_UART_WriteByte(uint8_t txData);
void DVRT_WritePacket(void);
void DVRT_Error_Callback(void);

const DVRT_interface_t DVRT = {
	.Initialize = DVRT_Initialize,
	.Process = DVRT_Process,
};


//------------------------------------------------------------------------------
/**
  Section: Variables
*/

static DVRT_VariablePointerTableEntry_t DVPMT[DYNAMIC_VAR_PTR_COUNT];		/* Dynamic Variable Pointer Monitor Array */

static volatile union DVCmds {
    DVRT_StreamUpdates_t stream;
    DVRT_VariableUpdate_t Var;
    DVRT_StreamIntervalUpdate_t interval;
	DVRT_CommandTemplate_t generic;
    uint8_t DVCmdArray[sizeof (DVRT_StreamUpdates_t)];
} DVRT_ReceivedCmd;								

static volatile const uint16_t dvIdFw = DV_FW_CODE;
static volatile uint8_t rxBufPtr;
static volatile uint8_t DVFrameCounter;
static uint16_t DVStreamInterval;
static uint16_t DVStreamInterval_Counter;
static uint16_t DVCmdInterval;
static uint16_t DVCmdInterval_Counter;
static size_t USART_Error; 

static struct flagS{
    unsigned streamOn	: 1;	/* Streaming On */
	unsigned osr		: 1;	/* One shot reading */
	unsigned ping		: 1;	/* Ping target microcontroller */
} DVflag;


/**
 * @ingroup dvruntime
 * @brief This blocking function writes a byte of data to selected UART.
 * @pre The transfer status is checked to see if the transmitter is ready to accept a byte before UART->Write()
 * @param txData - Data byte to write to the TX FIFO.
 * @return None.
 */
void DVRT_UART_WriteByte(uint8_t txData){
	while(!UART_DV->IsTxReady()){
	};
	UART_DV->Write(txData);
}

void DVRT_WritePacket(void){
	uint8_t i;
	uint8_t k;
	DVRT_UART_WriteByte(DV_START_OF_FRAME);
	DVRT_UART_WriteByte(DVFrameCounter++);
	if(DVflag.ping){
		DVRT_UART_WriteByte((uint8_t)(DV_FW_CODE));
		DVRT_UART_WriteByte((uint8_t)(DV_FW_CODE>>8));
	}
	else{
		for (i = 0; i < DYNAMIC_VAR_PTR_COUNT; i++) {
			if (DVPMT[i].address != NULL) { // Null check for safety
				for (k = 0; k < DVPMT[i].size; k++) {
					DVRT_UART_WriteByte(DVPMT[i].address[k]); // Use array indexing
				}
			}		
		}		
	}		
	DVRT_UART_WriteByte(DV_END_OF_FRAME);
}


void DVRT_Initialize(void) {
	uint8_t i;
	for (i = 0; i < DYNAMIC_VAR_PTR_COUNT; i++) {
        DVPMT[i].address = NULL;
        DVPMT[i].size = 0;
    }
	
    DVflag.streamOn = 1;
	DVStreamInterval = DV_STREAM_TIME;
	DVCmdInterval = DV_RX_CMD_TIMEOUT;

	error = 0;
	UART_DV->FramingErrorCallbackRegister(&DVRT_Error_Callback);
	UART_DV->OverrunErrorCallbackRegister(&DVRT_Error_Callback);
	TIMER_DVRT->TimeoutCallbackRegister(&DVRT_Process); 
}


void DVRT_Error_Callback(void)
{
	error = 1;
}


void DVRT_Process(void){

	if(error != 0U){
		USART_Error = UART1_ErrorGet();
		rxBufPtr = 0;
		error = 0;
	}
	else{
		while(UART_DV->IsRxReady() != 0U) {
		DVRT_ReceivedCmd.DVCmdArray[rxBufPtr] = UART_DV->Read();
		rxBufPtr++;
		}
	} 	

	if(DVCmdInterval_Counter++ >= DVCmdInterval){
		DVCmdInterval_Counter = 0;
		DVRT_HandleCommand();		    
	}	
	
	if(DVStreamInterval_Counter++ >= DVStreamInterval){
		DVStreamInterval_Counter = 0;
		if(DVflag.streamOn){				
			DVRT_WritePacket();
		}
	}

	if(DVflag.osr || DVflag.ping){			// One shot reading or ping command execution
		DVflag.osr = 0;
		DVflag.ping = 0;
		DVflag.streamOn = 0;				// stop streaming
	}	
	
}

void DVRT_HandleCommand(void) {
    uint8_t VARcount;
	if(rxBufPtr >= DV_RX_CMD_MIN_SIZE){
		if((DVRT_ReceivedCmd.DVCmdArray[0U] == DV_START_OF_FRAME) && (DVRT_ReceivedCmd.DVCmdArray[rxBufPtr-1U] == DV_END_OF_FRAME)){
			switch (DVRT_ReceivedCmd.stream.command) {
				case UPDATE_VARIABLE_POINTER_TABLE:
				{
					VARcount = 0;
					while (VARcount < DVRT_ReceivedCmd.stream.size) {
						DVPMT[VARcount].size = DVRT_ReceivedCmd.stream.DVPMT[VARcount].size;
						DVPMT[VARcount].address = DVRT_ReceivedCmd.stream.DVPMT[VARcount].address;
						VARcount++;
					}
					break;
				}
				case UPDATE_VARIABLE_VALUE:
				{
					VARcount = 0;
					while (VARcount < DVRT_ReceivedCmd.Var.variablePointerTableSize) {
						((uint8_t *)DVRT_ReceivedCmd.Var.variableAddress)[VARcount] = DVRT_ReceivedCmd.Var.variableValue[VARcount];
        				VARcount++;
					}
					break;				
				}	
				case UPDATE_STREAMING_TICK:
				{
						DVStreamInterval = DVRT_ReceivedCmd.interval.period;
						DVStreamInterval_Counter = 0;
						break;
				}
				case TURN_STREAMING_OFF:
				{
						DVflag.streamOn = 0;
						break;
				}
				case TURN_STREAMING_ON:
				{
						DVflag.streamOn = 1;
						DVStreamInterval_Counter = DVStreamInterval;	 
						break;
				}
				case ONE_SHOT_READ:
				{
						DVflag.osr = 1;
						DVflag.streamOn = 1;
						DVStreamInterval_Counter = DVStreamInterval;	 
						break;
				}
				case PING:
				{
						DVflag.ping = 1;
						DVflag.streamOn = 1;
						DVStreamInterval_Counter = DVStreamInterval;	 
						break;
				}	
				default:
				{
					// Handle unexpected command
					break;
				}
			}
		} 
	}
	rxBufPtr = 0;
}