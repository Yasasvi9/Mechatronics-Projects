//Author: Anaf Mahbub


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "wait.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "clock.h"

#define DATA PORTC,6
#define PD_CLK PORTC,7

uint32_t data = 0;

void initHw(){

    initSystemClockTo40Mhz();
    enablePort(PORTC);
    _delay_cycles(3);

    selectPinDigitalInput(DATA);
    enablePinPulldown(DATA);
    selectPinPushPullOutput(PD_CLK);
}

// main
void main(void)
{
    initHw();
    initUart0();
    setUart0BaudRate(115200, 40e6);
    uint8_t i;
    char str[40];
    float mass = 0, force = 0;


    while(true)
    {
        if(getPinValue(DATA) == 1)
        {
            data = 0;
            for (i = 0; i < 24; i++)
            {
                setPinValue(PD_CLK, 1);
                waitMicrosecond(40);
                data <<= 1;
                data |= getPinValue(DATA);
                setPinValue(PD_CLK,0);
                waitMicrosecond(1);
            }

            setPinValue(PD_CLK, 1);
            waitMicrosecond(40);
            setPinValue(PD_CLK,0);


            snprintf(str,sizeof(str),"Raw Data: %d", data);
            putsUart0(str);
            putsUart0("\n");
            mass = abs(data - 12001104);
            snprintf(str, sizeof(str), "Mass: %f", (mass / 13) - 175);
            putsUart0(str);
            putsUart0("\n");
            snprintf(str,sizeof(str),"Force: %f", mass*9.81);
            putsUart0(str);
            putsUart0("\n\n");



            waitMicrosecond(1000000);

        }
    }
}
