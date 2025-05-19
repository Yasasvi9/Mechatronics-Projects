// Timing C/ASM Mix Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Green LED:
//   PF3 drives an NPN transistor that powers the green LED

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <clock.h>
#include <stdint.h>
#include <stdbool.h>
#include "wait.h"
#include "tm4c123gh6pm.h"

// Bitband alias
#define OUTPUT (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 4*4)))             // PC 4
#define GREEN_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))             // PF 3

// PortF masks
#define  OUTPUT_MASK 16           // PC 3
#define  GREEN_LED_MASK 8           // PF 3

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Enable clocks
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;        // PORTF
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;        // PORTC
    _delay_cycles(3);

    // Configure LED pin
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK;  // make bit an output
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK;  // enable LED

    GPIO_PORTC_DIR_R |= OUTPUT_MASK;  // make bit an output
    GPIO_PORTC_DEN_R |= OUTPUT_MASK;  // enable OUTPUT
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
	// Initialize hardware
	initHw();

    // Toggle green LED every 10 micro-seconds
	while(true)
	{
      OUTPUT = 1;
//      GREEN_LED ^= 1;
      waitMicrosecond(65000);        // WAIT FOR 10us
      OUTPUT = 0;
      waitMicrosecond(100000);        // WAIT FOR 10us
	}
}
