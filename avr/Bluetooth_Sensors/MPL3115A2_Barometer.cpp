#include "MPL3115A2_Barometer.h"

// Class to provide and interface for the MPL3115A2 barometric pressure and altitude sensor. 
// The sensor provides conversion from altitude to pressure.

// The slave device address
#define DEVICE_ADDRESS 0x60

// Register addresses from Freescales Datasheets
#define STATUS 0x00
#define OUT_P_MSB 0x01
#define OUT_P_CSB 0x02
#define OUT_P_LSB 0x03
#define OUT_T_MSB 0x04
#define OUT_T_LSB 0x05

#define WHO_AM_I   0x0C

#define CTRL_REG1 0x26
#define CTRL_REG2 0x27
#define CTRL_REG3 0x28
#define CTRL_REG4 0x29
#define CTRL_REG5 0x2B

// Register constant values from Freescales Datasheets
//#define WHO_AM_I_VALUE 0x0D
#define WHO_AM_I_VALUE 0xC4

// Bitmasks required to set register values from Freescale Datasheets
#define ENABLE_FILTER 0x10
#define ACTIVE 0x01
#define RESET 0x04
#define ALTIMETER_MODE 0x80

// Error handeling codes
#define NO_ERROR 0
#define BUFFER_SIZE_ERROR 1
#define ADDRESS_NO_ACKNOWLEDGE 2
#define DATA_NO_ACKNOWLEDGE 3
#define TWI_ERROR 4
#define IDENTIFICATION_FAILURE 5

// Initialization of the communication and sensor hardware
uint8_t MPL3115A2_Barometer::setup()
{

    // Register and error code state
    uint8_t reg_value, error;
    
    // Start the I2C connection
    Wire.begin();

    // Get the devices identity
    error = readRegister(DEVICE_ADDRESS, WHO_AM_I, reg_value);
    if (error != NO_ERROR)
	return error;

    // Make sure the device is the barometer
    if (reg_value != WHO_AM_I_VALUE)
    {
	error = IDENTIFICATION_FAILURE;
	return error;
    }

    // Reset the registers to default
    error = reset();
    if (error != NO_ERROR)
    	return error;

    // Power down the sampling hardware
    error = standby();
    if (error != NO_ERROR)
    	return error;

    // Set the output to altitude instead of pressure
    reg_value = ALTIMETER_MODE | ACTIVE;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
    	return error;

    // Resume the sampling
    error = resume();
    return error;
}

// Turn on the sampling hardware
uint8_t MPL3115A2_Barometer::resume()
{
    // Register and error code state
    uint8_t reg_value, error;

    // Save the other settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the active bit high
    reg_value = reg_value | ACTIVE;
    error =  writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    return error;
}


// Reset the registers to default state
uint8_t MPL3115A2_Barometer::reset()
{
    // Register and error code state
    uint8_t reg_value, error;

    // Save the other settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the reset bit high, this will automatically clear
    reg_value = reg_value | RESET;
    error =  writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);

    // The sensor resets before the connection is closed so handle the error here
    if (error == 3)
	error = 0;
    else
	return error;

    // This sensor needs time to reset
    delay(5);
    return error;
}

// Power down the sampling hardware
uint8_t MPL3115A2_Barometer::standby()
{
    // Register and error code state
    uint8_t reg_value, error;

    // Save the other settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the active bit low
    reg_value = reg_value & ~ACTIVE;
    return writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value); 
}

// Read and store the altitude and temperature data
uint8_t MPL3115A2_Barometer::readData()
{
    // Register and error code state
    uint8_t error, reg_value;

    // Loop through each axis using sequential byte offsets from the first axis high byte
    // The I2C implimentation is not compatible with Wirings buffered readRegister() function
    for(int i = 0; i < 5 ; i++)
    {
	error = readRegister(DEVICE_ADDRESS, OUT_P_MSB + i , reg_value);
	if (error != 0)
	    return error;
    	data[i] = reg_value;
    }
    // AVR architecture is little endian, but the sensor values are big endian
    // this operation switches them for the multibyte altitude
    uint8_t temp;
    temp = data[0];
    data[0] = data[1];
    data[1] = temp;
    // Shift the fractional bits over so they are aligned with the start of the byte
    pressure_frac >>= 4;
    temperature_frac >>= 4;
    return error;
}

// Send the altitude and temperature data with the specified serial device in byte sized
// chuncks with a format of altitude integer high and low bytes, altitude fractional byte
// termperature integer byte and temperature fractional byte (the fractional bytes are in
// 4 bit fixed point format)
void MPL3115A2_Barometer::sendData(HardwareSerial & serial_device, byte delimiter)
{
    for (int i = 0; i < 5; i++)
    {
	if (data[i] == delimiter)
	    serial_device.write(delimiter);
	serial_device.write(data[i]);
    }
}



