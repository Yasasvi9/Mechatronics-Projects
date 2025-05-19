#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "clock.h"
#include "commands.h"
#include "gpio.h"
#include "stringf.h"
#include "uart0.h"
#include "wait.h"
#include "tm4c123gh6pm.h"

#define EN1 (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 4*4)))               // PC 4
#define EN2 (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 5*4)))               // PC 5
#define PF0 (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 7*4)))               // PC 7 photo diode
#define PE5 (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 5*4)))               // PE 5
#define PF1 (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))               // PF 1
#define PF2 (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))               // PF 2
#define PF3 (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))               // PF 3


#define PB4_MASK 16             // PB 4 IR LED
#define EN1_MASK 16             // PC 4
#define EN2_MASK 32             // PC 5
#define OE_MASK  128            // PC 7 photo diode
#define PF1_MASK 2              // PE 5
#define PF2_MASK 4              // PF 1
#define PF3_MASK 8              // PF 2
#define PE5_MASK 32             // PF 3

uint8_t currentPhase, motorfactor;
float *sinTable = NULL, *cosTable = NULL;
float error, currentAngle;


// INIT HW
void initHw()
{
    initSystemClockTo40Mhz();
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;                    // Runtime clocks for PORT B
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;                    // Runtime clocks for PORT C
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;                    // Runtime clocks for PORT E
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;                    // Runtime clocks for PORT F
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;                      // Enable PWM1 module

    _delay_cycles(3);

    GPIO_PORTC_DIR_R &= ~(OE_MASK);
    GPIO_PORTC_DEN_R |= EN1_MASK | EN2_MASK | OE_MASK;

    GPIO_PORTE_DIR_R |= PE5_MASK;
    GPIO_PORTE_DEN_R |= PE5_MASK;

    GPIO_PORTF_DIR_R |= PF1_MASK | PF2_MASK | PF3_MASK;
    GPIO_PORTF_DEN_R |= PF1_MASK | PF2_MASK | PF3_MASK;

    GPIO_PORTB_DIR_R |= PB4_MASK;
    GPIO_PORTB_DEN_R |= PB4_MASK;
    GPIO_PORTB_DATA_R |= PB4_MASK;

    // Configure the PWM pins (PC4 and PC5)
    GPIO_PORTC_AFSEL_R |= EN1_MASK | EN2_MASK;        // Enable alternate function for PC4 and PC5
    GPIO_PORTC_PCTL_R &= ~(GPIO_PCTL_PC4_M | GPIO_PCTL_PC5_M);  // Clear PCTL for PC4 and PC5
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC4_M0PWM6 | GPIO_PCTL_PC5_M0PWM7; // Configure PC4 as M1PWM6 and PC5 as M1PWM7

    // Reset the PWM module
    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0;                // Reset PWM0 module
    SYSCTL_SRPWM_R = 0;                              // Leave reset state

    // Configure PWM Generator 3 for M1PWM6 (PC4) and M1PWM7 (PC5)
    PWM0_3_CTL_R = 0;                                // Turn off PWM Generator 3 for configuration
    PWM0_3_GENA_R = PWM_3_GENA_ACTCMPAD_ONE | PWM_3_GENA_ACTLOAD_ZERO; // Set action for M1PWM6
    PWM0_3_GENB_R = PWM_3_GENB_ACTCMPBD_ONE | PWM_3_GENB_ACTLOAD_ZERO; // Set action for M1PWM7

    // Set the PWM frequency
    PWM0_3_LOAD_R = 1024;                            // Set frequency to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz

    // Initialize PWM signals to off (0% duty cycle)
    PWM0_3_CMPA_R = 0;                               // M1PWM6 (PC4) off
    PWM0_3_CMPB_R = 0;                               // M1PWM7 (PC5) off

    // Enable the PWM generators
    PWM0_3_CTL_R = PWM_3_CTL_ENABLE;                 // Turn on PWM Generator 3
    PWM0_ENABLE_R = PWM_ENABLE_PWM6EN | PWM_ENABLE_PWM7EN; // Enable PWM outputs on PC4 and PC5

    initUart0();
    setUart0BaudRate(115200, 40e6);
}

void microstep(float target)
{
    float S = 5;
    target = target - currentAngle;
    currentAngle += target;
//    double i = target * 10; //number of electrical degrees
    float i = fabs(target / 0.02);
    double stepPos = 0;
    while(i >= 0)
    {
        if(target >= 0)
        {
            if(stepPos == 90)
            {
                stepPos = 0;
                if(currentPhase == 3)
                {
                    currentPhase = 0;
                } else
                {
                    currentPhase++;
                }
                rotatePhase1(currentPhase);
            }
            //start working
            setPwmDutyCycle(fabs(1023*cos((stepPos+S)*M_PI/180.0)),fabs(1023*sin((stepPos+S)*M_PI/180.0)));
            stepPos += S;
        } else
        {
            if(stepPos == 0)
            {
                stepPos = (90);
                if(currentPhase == 0)
                {
                    currentPhase = 3;
                } else
                {
                    currentPhase--;
                }
                rotatePhase1(currentPhase);
            }
            //start working
            setPwmDutyCycle(fabs(1023*cos((stepPos-S)*M_PI/180.0)),fabs(1023*sin((stepPos-S)*M_PI/180.0)));
            stepPos -= S;
        }
        waitMicrosecond(350);
        i = i - S;
    }
    return;
}
// main
void main()
{
    initHw();
    setPwmDutyCycle(1023, 1023);  // Start with full torque (full step)
    currentPhase = calibrate();    // Calibrate to initial phase
    currentAngle = 0;              // Initial angle set to 0
    motorfactor = 1;               // Default motor factor (full step)

    SHELL_DATA shellCommand;       // Shell data structure for commands

    while(true)
    {
        if(kbhitUart0())           // Check if a command is entered
        {
            getsUart0(&shellCommand);
            putsUart0(shellCommand.buffer);
            parseFields(&shellCommand);
            putsUart0("\n");

            // Calibrate command
            if(isCommand(&shellCommand, "calibrate", 0))
            {
                currentPhase = calibrate(); // Perform motor calibration
                currentAngle = 0;           // Reset current angle
            }
            // Angle command: move motor to a specific angle
            else if(isCommand(&shellCommand, "angle", 1))
            {
                float angle = getFieldFloat(&shellCommand, 1);  // Parse angle input
                error = calculateError(angle, currentAngle);    // Calculate error
                uint8_t steps = (uint8_t) (abs((error)) / (1.8 / motorfactor));
                if(motorfactor == 1)
                {
                    if(error > 0)
                    {
                        currentPhase = clockWise(steps, currentPhase);                     //move clockwise
                    }
                    else if(error < 0)
                    {
                        currentPhase = cclockWise(steps, currentPhase);                    //move counter clockwise
                    }

                }
                else if(motorfactor == 32)
                {
                    microstep(angle);
                }
                currentAngle = angle;        // Update current angle
                putsUart0("\n");
            }
            // Step command: set the motor factor (1, 2, 4, 32)
            else if(isCommand(&shellCommand, "step", 1))
            {
                motorfactor = getFieldInteger(&shellCommand, 1); // Parse motorfactor
                if (motorfactor == 1 || motorfactor == 2 || motorfactor == 4 || motorfactor == 32)
                {
                    putsUart0("Motor factor set successfully.\n");
                }
                else
                {
                    putsUart0("Invalid motor factor. Use 1, 2, 4, or 32.\n");
                }
            }
            // Help command: display the available commands
            else if(isCommand(&shellCommand, "help", 0))
            {
                putsUart0("Here are the list of available commands:\n");
                putsUart0("-> calibrate  -- Calibrate the motor to zero position.\n");
                putsUart0("-> angle      -- Sets the motor to the desired angle (positive for CW, negative for CCW).\n");
                putsUart0("-> step       -- Set the Motor Factor to enable Micro-stepping (options: 1, 2, 4, 32).\n");
            }
        }
    }
}
