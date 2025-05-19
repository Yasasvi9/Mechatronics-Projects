//Anaf Mahbub
// Yasasvi Vanapalli
// I2C Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz
// Stack:           4096 bytes (needed for snprintf)

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

// I2C devices on I2C bus 0 with 2kohm pullups on SDA (PB3) and SCL (PB2)

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "uart0.h"
#include "i2c0.h"
#include "wait.h"

// Range of polled devices
#define I2C_MIN_ADDR 0x08
#define I2C_MAX_ADDR 0x77

#define SENSOR_ADDR 0x48
#define REG_CONFIG_ADDR 0x01
#define REG_DATA_ADDR 0x00

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------
typedef struct tempVoltMapping
{
    float tempValue;
    float voltValue;
} tempVoltMap;

const tempVoltMap conversionTable[] = {
    {-90.0, -3.243},
    {-80.0, -2.920},
    {-70.0, -2.587},
    {-60.0, -2.243},
    {-50.0, -1.889},
    {-40.0, -1.527},
    {-30.0, -1.156},
    {-20.0, -0.778},
    {-10.0, -0.392},
    {0.0, 0.000},
    {10.0, 0.397},
    {20.0, 0.798},
    {30.0, 1.203},
    {40.0, 1.612},
    {50.0, 2.023},
    {60.0, 2.436},
    {70.0, 2.851},
    {80.0, 3.267},
    {90.0, 3.682},
    {100.0, 4.096},
    {110.0, 4.509},
    {120.0, 4.920},
    {130.0, 5.328},
    {140.0, 5.735},
    {150.0, 6.138},
    {160.0, 6.540},
    {170.0, 6.941},
    {180.0, 7.340},
    {190.0, 7.739},
    {200.0, 8.138},
    {210.0, 8.539},
    {220.0, 8.940},
    {230.0, 9.343},
    {240.0, 9.747},
    {250.0, 10.153},
    {260.0, 10.561},
    {270.0, 10.971},
    {280.0, 11.382},
    {290.0, 11.795},
    {300.0, 12.209}
};

#define TABLE_SIZE (sizeof(conversionTable) / sizeof(conversionTable[0]))

float linearInterpolate(float xValue, float x0, float x1, float y0, float y1)
{
    return y0 + (xValue - x0) * (y1 - y0) / (x1 - x0);
}

float convertTempToVoltage(float temperature)
{
    for (uint16_t index = 0; index < (TABLE_SIZE - 1); ++index)
    {
        if (temperature <= conversionTable[index + 1].tempValue)
            return linearInterpolate(temperature, conversionTable[index].tempValue, conversionTable[index + 1].tempValue,
                                     conversionTable[index].voltValue, conversionTable[index + 1].voltValue);
    }
    return 0.0; // Default if not in range
}

float convertVoltageToTemp(float voltage)
{
    for (uint16_t index = 0; index < (TABLE_SIZE - 1); ++index)
    {
        if (voltage <= conversionTable[index + 1].voltValue)
            return linearInterpolate(voltage, conversionTable[index].voltValue, conversionTable[index + 1].voltValue,
                                     conversionTable[index].tempValue, conversionTable[index + 1].tempValue);
    }
    return 0.0; // Default if not in range
}

// Initialize Hardware
void initializeHardware() {
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

#define MAX_BUFFER_SIZE 80

int main(void)
{
    char outputString[MAX_BUFFER_SIZE];
    uint8_t dataSize = 2;
    uint8_t i2cBuffer[2];
    float sensorReading;
    float compensatedVoltage = 0.0;
    uint16_t rawData = 0;

    // Initialize hardware
    initializeHardware();
    initUart0();
    initI2c0();

    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);

    putsUart0("I2C0 Utility\n");

    while (true)
    {
        i2cBuffer[0] = 0xE5;
        writeI2c0Registers(SENSOR_ADDR, REG_CONFIG_ADDR, i2cBuffer, dataSize);
        waitMicrosecond(1000000);
        readI2c0Registers(SENSOR_ADDR, REG_DATA_ADDR, i2cBuffer, dataSize);

        sensorReading = (int16_t)(((uint16_t)i2cBuffer[0] << 8) | i2cBuffer[1]) * 0.0625;
        snprintf(outputString, sizeof(outputString), "TMP36 Voltage (mV): %f\n", sensorReading);
        putsUart0(outputString);

        compensatedVoltage = convertTempToVoltage((sensorReading - 500) / 10);

        i2cBuffer[0] = 0x8F;
        writeI2c0Registers(SENSOR_ADDR, REG_CONFIG_ADDR, i2cBuffer, dataSize);
        waitMicrosecond(1000000);
        readI2c0Registers(SENSOR_ADDR, REG_DATA_ADDR, i2cBuffer, dataSize);

        sensorReading = ((int16_t)(((uint16_t)i2cBuffer[0] << 8) | i2cBuffer[1])) * 0.0078125;
        rawData = (((uint16_t)i2cBuffer[0] << 8) | i2cBuffer[1]);

        if (rawData == 0x7FFF)
            putsUart0("Thermocouple disconnected\n");

        snprintf(outputString, sizeof(outputString), "ADS Voltage (mV): %f\n", sensorReading);
        putsUart0(outputString);

        snprintf(outputString, sizeof(outputString), "AIN2 raw voltage (mV): %f\n", sensorReading + compensatedVoltage);
        putsUart0(outputString);

        snprintf(outputString, sizeof(outputString), "Temperature (deg C): %f\n\n", convertVoltageToTemp(sensorReading + compensatedVoltage) - 4);
        putsUart0(outputString);
    }
}
