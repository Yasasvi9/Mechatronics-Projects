/*
 * uart1.h
 *
 *  Created on: Nov 27, 2024
 *      Author: vrrya
 */

#ifndef UART1_H_
#define UART1_H_

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initUart1();
void setUart1BaudRate(uint32_t baudRate, uint32_t fcyc);
void putcUart1(char c);
void putsUart1(char* str);
char getcUart1();
bool kbhitUart1();

#endif /* UART1_H_ */
