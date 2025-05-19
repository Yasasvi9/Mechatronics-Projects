/*
 * commands.c
 *
 *  Created on: Sep 20, 2024
 *      Author: Yasasvi Vanapalli
 */

#include <stdint.h>
#include <stdbool.h>
#include "commands.h"
#include "gpio.h"
#include "wait.h"
#include <math.h>
#include "tm4c123gh6pm.h"

#define S 5 //number of electrical degrees per micro-step was previously 5

// Set PWM Duty Cycle
void setPwmDutyCycle(uint16_t dutyCycle1, uint16_t dutyCycle2)
{
    PWM0_3_CMPA_R = dutyCycle1;  // Set duty cycle for EN1
    PWM0_3_CMPB_R = dutyCycle2;  // Set duty cycle for EN2
}

// Calculate the shortest rotation path
float calculateError(float targetAngle, float currentAngle)
{
    float error1 = targetAngle - currentAngle;
    if (error1 > 180)
        error1 -= 360;
    else if (error1 < -180)
        error1 += 360;
    return error1;
}

// rotation function
uint8_t rotatePhase1(uint8_t step)
{
    switch(step)
    {
    case 0:
        setPinValue(PORTE, 5, 1);               // A
        setPinValue(PORTF, 1, 0);               // B
        setPinValue(PORTF, 2, 0);               // C
        setPinValue(PORTF, 3, 0);               // D
        return step;

    case 1:
        setPinValue(PORTE, 5, 0);               // A
        setPinValue(PORTF, 1, 0);               // B
        setPinValue(PORTF, 2, 1);               // C
        setPinValue(PORTF, 3, 0);               // D
        return step;

    case 2:
        setPinValue(PORTE, 5, 0);               // A
        setPinValue(PORTF, 1, 1);               // B
        setPinValue(PORTF, 2, 0);               // C
        setPinValue(PORTF, 3, 0);               // D
        return step;

    case 3:
        setPinValue(PORTE, 5, 0);               // A
        setPinValue(PORTF, 1, 0);               // B
        setPinValue(PORTF, 2, 0);               // C
        setPinValue(PORTF, 3, 1);               // D
        return step;

    default:
        setPinValue(PORTE, 5, 1);               // A
        setPinValue(PORTF, 1, 0);               // B
        setPinValue(PORTF, 2, 0);               // C
        setPinValue(PORTF, 3, 0);               // D
        return step;

    }
}

// Calibrate the stepper motor
uint8_t calibrate()
{
    setPwmDutyCycle(1023, 1023);
    uint8_t j = 0;
    int8_t i = 0;
    while(1)           // while the photo diode doesn't read 0, keep rotating clockwise
    {
        rotatePhase1(i);                         // move clockwise
        i++;
        if(i>3)
        {
            i = 0;
        }
        waitMicrosecond(80000);
        if(getPinValue(PORTC, 7) == 1)
            break;
    }
    if(getPinValue(PORTC, 7) == 1)
    {
        if(i == 0)
        {

        }
        else
            i--;
        waitMicrosecond(80000);
        while(j<20)
        {
           rotatePhase1(i);                     // move counter clockwise
            i--;
            if(i<0)
            {
                i = 3;
            }
            j++;
            waitMicrosecond(80000);
        }
    }
    return i;
}

// Move Clockwise
uint8_t clockWise(uint8_t steps1, uint8_t currElecPhase1)
{
    setPwmDutyCycle(1023, 1023);
    uint8_t i = 0;
    int8_t j = currElecPhase1;
    while(i < steps1)
    {
        currElecPhase1 = rotatePhase1(j);
        j++;
        if(j > 3)
           j = 0;
        waitMicrosecond(500000);
        i++;
    }
    return currElecPhase1;
}

// Move Counter Clockwise
uint8_t cclockWise(uint8_t steps2, uint8_t currElecPhase2)
{
    setPwmDutyCycle(1023, 1023);
    int j = currElecPhase2;
    uint8_t i = 0;
    while(i < steps2)
    {
        currElecPhase2 = rotatePhase1(j);
        j--;
        if(j < 0)
            j = 3;
        waitMicrosecond(500000);
        i++;
    }
    return currElecPhase2;
}







