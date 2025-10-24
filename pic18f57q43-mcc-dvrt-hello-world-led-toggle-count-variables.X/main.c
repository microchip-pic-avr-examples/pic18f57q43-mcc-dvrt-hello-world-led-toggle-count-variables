 /*
 * MAIN Generated Driver File
 * 
 * @file main.c
 * 
 * @defgroup main MAIN
 * 
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.2
 *
 * @version Package Version: 3.1.2
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
#include "mcc_generated_files/system/system.h"

static volatile bool     LED_ON = false;
static volatile uint16_t LED_period_ms = 250U;
static volatile uint16_t Count_msTicksSinceToggle = 0U;

void Timer_Callback_1ms(void){
    Count_msTicksSinceToggle++;
    if(Count_msTicksSinceToggle >= LED_period_ms){
        LED_ON = !LED_ON;    // Toggle LED state variable
        IO_LED_Toggle();
        Count_msTicksSinceToggle = 0U;
    }
}

/*
    Main application
*/

int main(void)
{
    SYSTEM_Initialize();
    INTERRUPT_GlobalInterruptEnable();   /* For PIC18F */
    /* Comment out for AVR: Enable Global IRQ in Interrupt Manager */
    /* Comment out for dsPIC, Global IRQs are enabled by default */
    
    /* Replace X with number of 2nd timer added */ 
    /* From name of const struct TIMER_INTERFACE, path: mcc_generated_files\timer\src\timerX.c */
    Timer1.TimeoutCallbackRegister(Timer_Callback_1ms); 

    while(1)
    {
    }    
}
