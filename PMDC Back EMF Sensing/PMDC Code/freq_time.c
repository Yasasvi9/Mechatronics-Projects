// Frequency Counter / Timer Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz
// Stack:           4096 bytes (needed for snprintf)

// Hardware configuration:
// Green LED:
//   PF3 drives an NPN transistor that powers the green LED
// Blue LED:
//   PF2 drives an NPN transistor that powers the blue LED
// Pushbutton:
//   SW1 pulls pin PF4 low (internal pull-up is used)
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1
// Frequency counter and timer input:
//   SIGNAL_IN on PC6 (WT1CCP0)

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "clock.h"
#include "gpio.h"
#include "wait.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "adc.h"


#define RED_LED         (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define GREEN_LED       (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
#define BLUE_LED        (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))
#define PUSH_BUTTON     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 4*4)))                      // PF4
#define PUSH_BUTTON_2   (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 0*4)))                      // PF0
#define EN1             (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 4*4)))                      // PC 4

// PortC masks
#define EN1_MASK 16             // PC 4
#define FREQ_IN_MASK 64

// PortF masks
#define BLUE_LED_MASK 4
#define GREEN_LED_MASK 8
#define PUSH_BUTTON_MASK 16
#define PUSH_BUTTON_2_MASK 1
#define PB4_MASK 16             // PB 4 IR LED

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

bool timeMode = false;
uint32_t frequency = 0;
uint32_t time = 0;
uint32_t rpm = 0;
uint32_t rawEMF = 0;
float InverseBEMF = 0;
float ActualBEMF = 0;
float BEMFrpm = 0;
uint16_t x = 0;
char str[40];
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------
// Set the PWM Duty Cycle
uint16_t setPwmDutyCycle(uint16_t dutyCycle)
{
    PWM0_3_CMPA_R = dutyCycle;  // Set duty cycle for EN1
    return dutyCycle;
}

void enableCounterMode()
{
    // Configure Timer 1 as the time base
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    TIMER1_TAILR_R = 40000000;                       // set load value to 40e6 for 1 Hz interrupt rate
    TIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    NVIC_EN0_R |= (1 << (INT_TIMER1A-16));

    // Configure Wide Timer 1 as counter of external events on CCP0 pin
	WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
	WTIMER1_CFG_R = 4;                               // configure as 32-bit counter (A only)
	WTIMER1_TAMR_R = TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR; // configure for edge count mode, count up
    WTIMER1_CTL_R = TIMER_CTL_TAEVENT_POS;           // count positive edges
    WTIMER1_IMR_R = 0;                               // turn-off interrupts
	WTIMER1_TAV_R = 0;                               // zero counter for first period
    WTIMER1_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter
}


// Frequency counter service publishing latest frequency measurements every second
void timer1Isr()
{
    frequency = WTIMER1_TAV_R;                   // read counter input
    rpm = (frequency / 32) * 60;


    snprintf(str, sizeof(str), "Frequency: %7"PRIu32" (Hz)\n", frequency);
    putsUart0(str);
    snprintf(str, sizeof(str), "RPM: %7"PRIu32" \n", rpm);
    putsUart0(str);

    setPwmDutyCycle(0);
    waitMicrosecond(1000);
    rawEMF = readAdc0Ss1();
    InverseBEMF = ((rawEMF + 0.5)/4096) * 3.3;
    ActualBEMF = 10 - (InverseBEMF * 5.7);
    BEMFrpm = (541.9 * ActualBEMF) + 2176.4;
    setPwmDutyCycle(x);

    snprintf(str, sizeof(str), "Raw BEMF: %7"PRIu32" \n", rawEMF);
    putsUart0(str);
    snprintf(str, sizeof(str), "Inverse BEMF: %7.2f \n", InverseBEMF);
    putsUart0(str);
    snprintf(str, sizeof(str), "Actual BEMF: %7.2f \n", ActualBEMF);
    putsUart0(str);
    snprintf(str, sizeof(str), "PWM: %7"PRIu32"  \n", (x/10));
    putsUart0(str);
    snprintf(str, sizeof(str), "BEMF Rpm: %7.2f \n", BEMFrpm);
    putsUart0(str);
    snprintf(str, sizeof(str), "\n\n");
    putsUart0(str);



    WTIMER1_TAV_R = 0;                           // reset counter for next period
    BLUE_LED ^= 1;                               // status
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;           // clear interrupt flag
}
// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Enable clocks
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1 | SYSCTL_RCGCGPIO_R2 | SYSCTL_RCGCGPIO_R5;
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;
    _delay_cycles(3);

    setPinCommitControl(PORTF, 0);

    // Configure LED and pushbutton pins
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK | BLUE_LED_MASK;  // bits 1 and 2 are outputs, other pins are inputs
    GPIO_PORTF_DIR_R &= ~(PUSH_BUTTON_MASK | PUSH_BUTTON_2_MASK);               // bit 4 is an input
    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK | BLUE_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R |= PUSH_BUTTON_MASK | PUSH_BUTTON_2_MASK | GREEN_LED_MASK | BLUE_LED_MASK;
                                                         // enable LEDs and pushbuttons
    GPIO_PORTF_PUR_R |= PUSH_BUTTON_MASK | PUSH_BUTTON_2_MASK;                // enable internal pull-up for push button

    //Configure IR-LED
    GPIO_PORTB_DIR_R |= PB4_MASK;
    GPIO_PORTB_DEN_R |= PB4_MASK;
    GPIO_PORTB_DATA_R |= PB4_MASK;

    // Configure SIGNAL_IN for frequency and time measurements
    GPIO_PORTC_AFSEL_R |= FREQ_IN_MASK | EN1_MASK;              // select alternative functions for SIGNAL_IN pin
    GPIO_PORTC_PCTL_R &= ~GPIO_PCTL_PC6_M;                      // map alt fns to SIGNAL_IN
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC6_WT1CCP0;
    GPIO_PORTC_DEN_R |= FREQ_IN_MASK | EN1_MASK;                // enable bit 6 for digital input

    // Configure the PWM pins (PC4)
    GPIO_PORTC_PCTL_R &= ~(GPIO_PCTL_PC4_M);  // Clear PCTL for PC4 and PC5
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC4_M0PWM6; // Configure PC4 as M1PWM6 and PC5 as M1PWM7

    // Reset the PWM module
    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0;                // Reset PWM0 module
    SYSCTL_SRPWM_R = 0;                              // Leave reset state

    // Configure PWM Generator 3 for M1PWM6 (PC4)
    PWM0_3_CTL_R = 0;                                // Turn off PWM Generator 3 for configuration
    PWM0_3_GENA_R = PWM_3_GENA_ACTCMPAD_ONE | PWM_3_GENA_ACTLOAD_ZERO; // Set action for M1PWM6

    // Set the PWM frequency
    PWM0_3_LOAD_R = 1024;                            // Set frequency to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz

    // Initialize PWM signals to off (0% duty cycle)
    PWM0_3_CMPA_R = 0;                               // M1PWM6 (PC4) off

    // Enable the PWM generators
    PWM0_3_CTL_R = PWM_3_CTL_ENABLE;                 // Turn on PWM Generator 3
    PWM0_ENABLE_R = PWM_ENABLE_PWM6EN; // Enable PWM outputs on PC4 and PC5
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    // Initialize hardware
    initHw();
    initUart0();
    initAdc0Ss1();
    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);


    // Setup the initial PWM duty cycle to 50%
    x = setPwmDutyCycle(0);

    enableCounterMode();


    // Endless loop performing multiple tasks
    while(true)
    {
        if(!PUSH_BUTTON)
        {
            x -= 102;
            {
                if(x<=102)
                    x = 0;
            }
            setPwmDutyCycle(x);
            waitMicrosecond(1000000);
        }
        else if(!PUSH_BUTTON_2)
        {
            x += 102;
            {
                if(x>=922)
                    x = 1023;
            }
            setPwmDutyCycle(x);
            waitMicrosecond(1000000);
        }
    }
}
