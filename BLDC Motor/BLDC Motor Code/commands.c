/*
 * commands.c
 *
 *  Created on: Oct 4, 2024
 *      Author: vrrya
 */
#include "gpio.h"
#include "wait.h"

// OPEN LOOP COMMUTATION
uint8_t electricalPhase(uint32_t speed)
{
    uint8_t phase;

    setPinValue(PORTF, 1, 1);           // A_en
    setPinValue(PORTF, 2, 1);           // B_en
    setPinValue(PORTF, 3, 0);           // C_en

    setPinValue(PORTC, 4, 1);           // A_dir
    setPinValue(PORTC, 5, 0);           // B_dir
    setPinValue(PORTC, 7, 0);           // C_dir

    uint8_t hallA = getPinValue(PORTD, 1);
    uint8_t hallB = getPinValue(PORTD, 2);
    uint8_t hallC = getPinValue(PORTD, 3);

    phase = 1;
    waitMicrosecond(speed);

    setPinValue(PORTF, 1, 0);           // A_en
    setPinValue(PORTF, 2, 1);           // B_en
    setPinValue(PORTF, 3, 1);           // C_en

    setPinValue(PORTC, 4, 0);           // A_dir
    setPinValue(PORTC, 5, 0);           // B_dir
    setPinValue(PORTC, 7, 1);           // C_dir

    hallA = getPinValue(PORTD, 1);
    hallB = getPinValue(PORTD, 2);
    hallC = getPinValue(PORTD, 3);

    phase = 2;
    waitMicrosecond(speed);

    setPinValue(PORTF, 1, 1);           // A_en
    setPinValue(PORTF, 2, 0);           // B_en
    setPinValue(PORTF, 3, 1);           // C_en

    setPinValue(PORTC, 4, 0);           // A_dir
    setPinValue(PORTC, 5, 0);           // B_dir
    setPinValue(PORTC, 7, 1);           // C_dir

    hallA = getPinValue(PORTD, 1);
    hallB = getPinValue(PORTD, 2);
    hallC = getPinValue(PORTD, 3);

    phase = 3;
    waitMicrosecond(speed);

    setPinValue(PORTF, 1, 1);           // A_en
    setPinValue(PORTF, 2, 1);           // B_en
    setPinValue(PORTF, 3, 0);           // C_en

    setPinValue(PORTC, 4, 0);           // A_dir
    setPinValue(PORTC, 5, 1);           // B_dir
    setPinValue(PORTC, 7, 0);           // C_dir

    hallA = getPinValue(PORTD, 1);
    hallB = getPinValue(PORTD, 2);
    hallC = getPinValue(PORTD, 3);

    phase = 4;
    waitMicrosecond(speed);

    setPinValue(PORTF, 1, 0);           // A_en
    setPinValue(PORTF, 2, 1);           // B_en
    setPinValue(PORTF, 3, 1);           // C_en

    setPinValue(PORTC, 4, 0);           // A_dir
    setPinValue(PORTC, 5, 1);           // B_dir
    setPinValue(PORTC, 7, 0);           // C_dir

    hallA = getPinValue(PORTD, 1);
    hallB = getPinValue(PORTD, 2);
    hallC = getPinValue(PORTD, 3);

    phase = 5;
    waitMicrosecond(speed);

    setPinValue(PORTF, 1, 1);           // A_en
    setPinValue(PORTF, 2, 0);           // B_en
    setPinValue(PORTF, 3, 1);           // C_en

    setPinValue(PORTC, 4, 1);           // A_dir
    setPinValue(PORTC, 5, 0);           // B_dir
    setPinValue(PORTC, 7, 0);           // C_dir

    hallA = getPinValue(PORTD, 1);
    hallB = getPinValue(PORTD, 2);
    hallC = getPinValue(PORTD, 3);

    phase = 6;
    waitMicrosecond(speed);

    return phase;
}

// CLOSED LOOP COMMUTATION
uint8_t commutate(uint8_t next)
{
    switch(next)
        {
        case 0b000:
            setPinValue(PORTF, 1, 0); // A_en
            setPinValue(PORTF, 2, 0); // B_en
            setPinValue(PORTF, 3, 0); // C_en
            return next;

        case 0b010:
            setPinValue(PORTF, 1, 0); // A_en
            setPinValue(PORTF, 2, 1); // B_en
            setPinValue(PORTF, 3, 1); // C_en

            setPinValue(PORTC, 4, 0); // A_dir
            setPinValue(PORTC, 5, 0); // B_dir
            setPinValue(PORTC, 7, 1); // C_dir
            return next;

        case 0b110:
            setPinValue(PORTF, 1, 1); // A_en
            setPinValue(PORTF, 2, 0); // B_en
            setPinValue(PORTF, 3, 1); // C_en

            setPinValue(PORTC, 4, 0); // A_dir
            setPinValue(PORTC, 5, 0); // B_dir
            setPinValue(PORTC, 7, 1); // C_dir
            return next;

        case 0b100:
            setPinValue(PORTF, 1, 1); // A_en
            setPinValue(PORTF, 2, 1); // B_en
            setPinValue(PORTF, 3, 0); // C_en

            setPinValue(PORTC, 4, 0); // A_dir
            setPinValue(PORTC, 5, 1); // B_dir
            setPinValue(PORTC, 7, 0); // C_dir
            return next;

        case 0b101:
            setPinValue(PORTF, 1, 0); // A_en
            setPinValue(PORTF, 2, 1); // B_en
            setPinValue(PORTF, 3, 1); // C_en

            setPinValue(PORTC, 4, 0); // A_dir
            setPinValue(PORTC, 5, 1); // B_dir
            setPinValue(PORTC, 7, 0); // C_dir
            return next;

        case 0b001:
            setPinValue(PORTF, 1, 1); // A_en
            setPinValue(PORTF, 2, 0); // B_en
            setPinValue(PORTF, 3, 1); // C_en

            setPinValue(PORTC, 4, 1); // A_dir
            setPinValue(PORTC, 5, 0); // B_dir
            setPinValue(PORTC, 7, 0); // C_dir
            return next;

        case 0b011:
            setPinValue(PORTF, 1, 1); // A_en
            setPinValue(PORTF, 2, 1); // B_en
            setPinValue(PORTF, 3, 0); // C_en

            setPinValue(PORTC, 4, 1); // A_dir
            setPinValue(PORTC, 5, 0); // B_dir
            setPinValue(PORTC, 7, 0); // C_dir
            return next;

//        case 0b100:
//            setPinValue(PORTF, 1, 1); // A_en
//            setPinValue(PORTF, 2, 0); // B_en
//            setPinValue(PORTF, 3, 1); // C_en
//
//            setPinValue(PORTC, 4, 1); // A_dir
//            setPinValue(PORTC, 5, 0); // B_dir
//            setPinValue(PORTC, 7, 0); // C_dir
//            return next;

        default:
            setPinValue(PORTF, 1, 0); // A_en
            setPinValue(PORTF, 2, 0); // B_en
            setPinValue(PORTF, 3, 0); // C_en
            break;
        }
    return next;
}















