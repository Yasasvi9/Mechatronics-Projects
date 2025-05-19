/*
 * commands.h
 *
 *  Created on: Sep 20, 2024
 *      Author: vrrya
 */

#ifndef COMMANDS_H_
#define COMMANDS_H_

float calculateError(float targetAngle, float currentAngle);
uint8_t rotatePhase1(uint8_t step);
uint8_t calibrate();
uint8_t clockWise(uint8_t steps1, uint8_t currElecPhase);
uint8_t cclockWise(uint8_t steps2, uint8_t currElecPhase2);
void setPwmDutyCycle(uint16_t dutyCycle1, uint16_t dutyCycle2);
#endif /* COMMANDS_H_ */
