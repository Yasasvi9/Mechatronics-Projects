
// Frequency Counter / Timer Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz
// Stack:           4096 bytes (needed for snprintf)


//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "clock.h"
#include "wait.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "gpio.h"

#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
#define BLUE_LED     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))


// PortF masks
#define BLUE_LED_MASK 4
#define GREEN_LED_MASK 8
#define RED_LED_MASK 2

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

uint32_t frequency = 0;
uint32_t time = 0;
uint32_t volume = 0;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Enable clocks
    SYSCTL_RCGCACMP_R |= SYSCTL_RCGCACMP_R0;
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2 | SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    selectPinAnalogInput(PORTC, 7);
    selectPinPushPullOutput(PORTC,4);

    COMP_ACREFCTL_R |= COMP_ACREFCTL_EN;
    COMP_ACREFCTL_R &= ~COMP_ACREFCTL_RNG;
    COMP_ACREFCTL_R |= COMP_ACREFCTL_VREF_M;

    COMP_ACCTL0_R |= COMP_ACCTL0_ASRCP_REF;

//    COMP_ACCTL0_R   = COMP_ACCTL0_TOEN;

    TIMER1_CTL_R  &= ~TIMER_CTL_TAEN;
    TIMER1_CFG_R   = 0x00000000;
    TIMER1_TAMR_R  = TIMER_TAMR_TAMR_1_SHOT;
    TIMER1_TAMR_R  = TIMER_TAMR_TACDIR;
    TIMER1_TAILR_R = 0xFFFFFFFF;

    // Configure LED and pushbutton pins
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK | BLUE_LED_MASK;  // bits 1 and 2 are outputs, other pins are inputs
    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK | BLUE_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK | BLUE_LED_MASK;

}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    // Initialize hardware
    initHw();
    initUart0();

    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);

    // Endless loop performing multiple tasks
    char str[40];
    while(true)
    {
        setPinValue(PORTC, 4, 1);
        waitMicrosecond(500000);
        setPinValue(PORTC, 4, 0);

        TIMER1_CTL_R |= TIMER_CTL_TAEN;

        while(COMP_ACSTAT0_R == 2);

        TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
        time = TIMER1_TAV_R;

        snprintf(str, sizeof(str), "Clocks: %7"PRIu32"\n", time);
        putsUart0(str);


        volume = (time * 2.584) - 1071.965;

        snprintf(str, sizeof(str), "Volume: %7"PRIu32"ml\n\n", volume);
        putsUart0(str);


        //reset tav value
        TIMER1_TAV_R = 0;

        waitMicrosecond(1000000);
    }
}
