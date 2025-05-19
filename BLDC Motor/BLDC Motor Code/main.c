#include <stdint.h>
#include <stdbool.h>
#include "clock.h"
#include "commands.h"
#include "gpio.h"
#include "nvic.h"
#include "stringf.h"
#include "uart0.h"
#include "wait.h"
#include "tm4c123gh6pm.h"

uint32_t i, currentPhase;

void initHw()
{
    initSystemClockTo40Mhz();
    enablePort(PORTF);
    enablePort(PORTC);
    enablePort(PORTD);
    _delay_cycles(3);

    // enable pins
    selectPinPushPullOutput(PORTF, 1);          // A_en
    selectPinPushPullOutput(PORTF, 2);          // B_en
    selectPinPushPullOutput(PORTF, 3);          // C_en

    // direction control pins
    selectPinPushPullOutput(PORTC, 4);          // A_dir
    selectPinPushPullOutput(PORTC, 5);          // B_dir
    selectPinPushPullOutput(PORTC, 7);          // C_dir

    // add the gpio pins for hall effect (PD 1,2,3)
    selectPinDigitalInput(PORTD, 1);            // H_A
    selectPinDigitalInput(PORTD, 2);            // H_B
    selectPinDigitalInput(PORTD, 3);            // H_C

    enablePinInterrupt(PORTD, 1);
    enablePinInterrupt(PORTD, 1);
    enablePinInterrupt(PORTD, 1);

    selectPinInterruptBothEdges(PORTD, 1);
    selectPinInterruptBothEdges(PORTD, 2);
    selectPinInterruptBothEdges(PORTD, 3);

    initUart0();
    setUart0BaudRate(115200, 40e6);
}

// GPIO Isr
void gpioIsr()
{
    uint8_t hallA = getPinValue(PORTD, 1);
    uint8_t hallB = getPinValue(PORTD, 2);
    uint8_t hallC = getPinValue(PORTD, 3);

    // Combine the states into a single byte
    uint8_t hallCode =  (hallA << 2) | (hallB << 1) | hallC;

    currentPhase = commutate(hallCode);

    clearPinInterrupt(PORTD, 1);
    clearPinInterrupt(PORTD, 2);
    clearPinInterrupt(PORTD, 3);
}

void main()
{
	initHw();
	SHELL_DATA shellCommand;
	i = 9000;

	while(1)
	{
	    // start off in closed loop commutation
        enableNvicInterrupt(19);

        uint8_t hallA = getPinValue(PORTD, 1);
        uint8_t hallB = getPinValue(PORTD, 2);
        uint8_t hallC = getPinValue(PORTD, 3);

        // Combine the states into a single byte
        uint8_t hallCode =  (hallA << 2) | (hallB << 1) | hallC;

        currentPhase = commutate(hallCode);

	    if(kbhitUart0())
	    {
	        getsUart0(&shellCommand);
	        putsUart0(shellCommand.buffer);
	        parseFields(&shellCommand);
	        putsUart0("\n");

	        if(isCommand(&shellCommand, "open", 0))
	        {
	            disableNvicInterrupt(19);
	            while(1)
	            {
	                currentPhase = electricalPhase(i);
	                if(i < 2950)
	                    i = 2950;
	                else
	                    i-=10;
	                if(kbhitUart0())
	                    break;
	            }
	        }
	        else if(isCommand(&shellCommand, "close", 0))
	        {
	            // closed loop commutation
	            enableNvicInterrupt(19);
	        }
	    }
	}
}
