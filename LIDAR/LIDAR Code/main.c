#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "wait.h"
#include "uart0.h"
#include "uart1.h"
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "clock.h"
#include <math.h>

#define MAX_SIZE 1000
#define MOTOR_MASK 16

typedef struct _lidarData
{
    uint16_t quality;
    float distance;
    float angle;
} lidarData;

uint8_t dataBuffer[20] = {0};
uint16_t i, j;
uint16_t k;
char printBuffer[50];

lidarData data[MAX_SIZE];

void initHw(){

    //Enable System Clock
    initSystemClockTo40Mhz();

    //Enable Registers
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;

    //Enable PWM Registers
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;
    _delay_cycles(3);

    // Configure Motor PWM
    GPIO_PORTC_DEN_R |= MOTOR_MASK;
    GPIO_PORTC_AFSEL_R |= MOTOR_MASK;
    GPIO_PORTC_PCTL_R &= ~(GPIO_PCTL_PC4_M);
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC4_M0PWM6;


    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0;
    SYSCTL_SRPWM_R = 0;
    PWM0_3_CTL_R = 0;

    PWM0_3_GENA_R = PWM_0_GENA_ACTCMPAD_ONE | PWM_0_GENA_ACTLOAD_ZERO;

    //Set load
    PWM0_3_LOAD_R = 1024;

    PWM0_3_CMPA_R = 0;

    PWM0_3_CTL_R = PWM_0_CTL_ENABLE;
    PWM0_ENABLE_R = PWM_ENABLE_PWM6EN;

    initUart0();
    initUart1();

    setUart0BaudRate(115200, 40e6);
    setUart1BaudRate(115200, 40e6);

}
void sendScan()
{
    putcUart1(0xA5);
    putcUart1(0x20);
}

void readResponse()
{
    for(i = 0; i < 7; i++)
    {
        dataBuffer[i] = (uint8_t) getcUart1();
    }
//    putsUart0("Descriptor Values:\t");
//    for (i = 0; i < 7; i++)
//    {
//        snprintf(printBuffer, sizeof(printBuffer), "%02X ", dataBuffer[i]); // Format each byte
//        putsUart0(printBuffer); // Print the formatted string to UART0
//    }
//    putsUart0("\r\n");
}

void processScanData(lidarData *data)
{
    i = 0;
    while(i < MAX_SIZE)
    {
        for(j = 0; j < 5; j++)
        {
            dataBuffer[j] = (uint8_t)getcUart1();
        }
        data[i].quality = dataBuffer[0];
        data[i].angle = (dataBuffer[1] >> 1 | (dataBuffer[2]) << 7) / 64.0;
        data[i].distance = (dataBuffer[3] | (dataBuffer[4] << 8)) / 4.0;

        if(data[i].quality == 0)
            continue;
        if(data[i].distance != 0)
            i++;
    }
}

// Bubble sort algorithm for sorting by angle
void sortData(lidarData *data)
{
    k = 0;
    bool swapped;
    do
    {
        swapped = false;
        for (k = 0; k < MAX_SIZE - 1; k++)
        {
            // Compare adjacent elements by angle
            if (data[k].angle > data[k + 1].angle)
            {
                // Swap the elements if out of order
                lidarData temp = data[k];
                data[k] = data[k + 1];
                data[k + 1] = temp;
                swapped = true;
            }
        }
    } while (swapped);
}

void sendStop()
{
    putcUart1(0xA5);
    putcUart1(0x25);
}

void getInfo()
{
    putcUart1(0xA5);
    putcUart1(0x50);
}

void printInfo()
{
    for(i = 0; i < 20; i++)
    {
        dataBuffer[i] = (uint8_t) getcUart1();
    }
    putsUart0("LIDAR Info Response:\t");
    for (i = 0; i < 20; i++)
    {
        snprintf(printBuffer, sizeof(printBuffer), "%02X ", dataBuffer[i]); // Format each byte
        putsUart0(printBuffer); // Print the formatted string to UART0
    }
    putsUart0("\r\n");
}

void calculateArea(lidarData *data)
{
    k = 0;
    float area = 0.0;

    // Iterate through sorted data to calculate the area
    for (k = 0; k < MAX_SIZE - 1; k++)
    {
        if((data[k].distance != 0) && (data[k].angle <= 360))
        {
            // Convert angles from degrees to radians
            float theta1 = data[k].angle * (M_PI / 180.0);
            float theta2 = data[k + 1].angle * (M_PI / 180.0);

            // Calculate the sector area contribution
            float deltaTheta = theta2 - theta1; // Difference in angle (radians)
            area += 0.5 * data[k].distance * data[k].distance * sin(deltaTheta);
        }
    }

    // Display the calculated area on UART0
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "Calculated Area: %.2f square inches\r\n", area*0.00155);
    putsUart0(buffer);
}
// main
void main(void)
{
    initHw();

    putsUart0("Power Up Motor Test......\n");
    PWM0_3_CMPA_R = 1023;
    waitMicrosecond(1000000);
    PWM0_3_CMPA_R = 0;
    putsUart0("Good to GO!.....\nSending the Stop Command.....\n");

    sendStop();

    waitMicrosecond(500000);

    putsUart0("Starting the Scan sequence, Please wait while data is being gathered and processed......\r\n");
    PWM0_3_CMPA_R = 1023;
    waitMicrosecond(2000000);

    sendScan();
    readResponse();
    processScanData(data);
    sendStop();                     // stop the lidar from further scans

    PWM0_3_CMPA_R = 0;
    sortData(data);
    putsUart0("Data received and Processed.......\r\nStarting the print sequence of Sorted Data......\r\n");
    //Print the data
    for (k = 0; k < MAX_SIZE; k++)
    {
        snprintf(printBuffer, sizeof(printBuffer), "%.2f, %.2f\n", data[k].angle, data[k].distance);
        putsUart0(printBuffer);
    }

    calculateArea(data);


    while(true);
}
