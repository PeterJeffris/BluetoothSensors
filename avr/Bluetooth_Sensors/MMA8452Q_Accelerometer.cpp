// Writen by Peter Jeffris, University of Colorado Mechanical Engineering

// Class to provide an interface to the MMA8452Q MEMS IC. This sensor provides acceleration values in three axes with dynamic filtering, range settings and power saving modes.

#include "MMA8452Q_Accelerometer.h"

// Default I2C Address of the Sparkfun MMA8542 Breakout, jumper toggle on chip bottom
#define DEVICE_ADDRESS 0x1D

// Register addresses from Freescales Datasheets
#define STATUS 0x00
#define OUT_X_MSB 0x01
#define OUT_X_LSB 0x02
#define OUT_Y_MSB 0x03
#define OUT_Y_LSB 0x04
#define OUT_Z_MSB 0x05
#define OUT_Z_LSB 0x06
#define WHO_AM_I   0x0D
#define XYZ_DATA_CFG  0x0E
#define HP_FILTER_CUTOFF 0x0F
#define CTRL_REG1 0x2A
#define CTRL_REG2 0x2B
#define CTRL_REG3 0x2C
#define CTRL_REG4 0x2D
#define CTRL_REG5 0x2F
#define INTERUPT 0x2E
#define OFFSET_X 0x30
#define OFFSET_Y 0x31
#define OFFSET_Z 0x31

// Register constant values from Freescales Datasheets
#define WHO_AM_I_VALUE 0x2A

// Error handeling codes
#define NO_ERROR 0
#define BUFFER_SIZE_ERROR 1
#define ADDRESS_NO_ACKNOWLEDGE 2
#define DATA_NO_ACKNOWLEDGE 3
#define TWI_ERROR 4
#define IDENTIFICATION_FAILURE 5

// Bitmasks required to set register values from Freescale Datasheets
#define ENABLE_FILTER 0x10
#define RANGE_2G 0x00
#define RANGE_4G 0x01
#define RANGE_8G 0x02
#define RANGE_MASK 0xFC
#define FILTER_16HZ 0x00
#define FILTER_8HZ 0x01
#define FILTER_4HZ 0x02
#define FILTER_2HZ 0x03
#define FILTER_MASK 0xFC
#define OUTPUT_800HZ 0x00
#define ACTIVE 0x01
#define LOW_NOISE 0x04
#define HIGH_RESOLUTION_MODE 0x02
#define LOW_POWER_SLEEP_MODE 0x18
#define AUTO_SLEEP 0x04
#define RESET 0x40

    // Initialization of the communication and sensor hardware
byte MMA8452Q_Accelerometer::setup()
{

    // Register and error code state
    byte reg_value, error;

    // Initialize I2C communication
    Wire.begin();

    // Get the identity of the device with the accelerometers address
    error = readRegister(DEVICE_ADDRESS, WHO_AM_I, reg_value);
    if (error != NO_ERROR)
	return error;

    // Make sure the device is actually the accelerometer
    if (reg_value != WHO_AM_I_VALUE)
    {
	error = IDENTIFICATION_FAILURE;
	return error;
    }

    // Reset the register values of the device
    error = reset();
    if (error != NO_ERROR)
    	return error;

    // Put the device into standby, turning off the hardware power (this is required)
    error = standby();
    if (error != NO_ERROR)
    	return error;

    error = setRange(MMA8452Q_Accelerometer::MAX_2G);
    if (error != NO_ERROR)
	return error;

    // Set the active mode to have high resolution and the sleep mode to optimize power
    reg_value = HIGH_RESOLUTION_MODE | LOW_POWER_SLEEP_MODE;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG2, reg_value);
    if (error != NO_ERROR)
    	return error;

    // Resume sampling
    error = resume();
    return error;
}

// Resume sampling
byte MMA8452Q_Accelerometer::resume()
{
    // Register and error code state
    byte reg_value, error;

    // Save the current settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the active flag high
    reg_value = reg_value | ACTIVE;
    error =  writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    return error;
}

//  Reset the register settings to default
byte MMA8452Q_Accelerometer::reset()
{
    // Register and error code state
    byte reg_value, error;

    // Save the current settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG2, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the reset flag high, this will automatically clear
    reg_value = reg_value | RESET;
    error =  writeRegister(DEVICE_ADDRESS, CTRL_REG2, reg_value);
    if (error != NO_ERROR)
	return error;
    delay(5);
    return error;
}


// Turn off the sampling hardware
byte MMA8452Q_Accelerometer::standby()
{
    // Register and error code state
    byte reg_value, error;

    // Save the current settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the active bit low
    reg_value = reg_value & ~ACTIVE;
    return writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value); 
}

// Enable mode to turn off the sampling hardware power if no requests are
// recived within a short timeout period
byte MMA8452Q_Accelerometer::enableSleepOnInactivity()
{
    // Register and error code state
    byte reg_value, error;

    // Save the current settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG2, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the sleep bit high
    reg_value = reg_value | AUTO_SLEEP;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG2, reg_value); 
    return error; 
}

// Disable sleep mode for responsive sampling at the expense of incresed power consumption
byte MMA8452Q_Accelerometer::disableSleepOnInactivity()
{
    // Register and error code state
    byte reg_value, error;

    // Save the current settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG2, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the sleep bit low
    reg_value = reg_value & ~AUTO_SLEEP;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG2, reg_value);
    return error;
}

// Enable high pass filtering on the output
byte MMA8452Q_Accelerometer::enableHighPassFilter()
{
    // Register and error code state
    byte reg_value, error;

    // Save the current settings
    error = readRegister(DEVICE_ADDRESS, XYZ_DATA_CFG, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the filter bit high
    reg_value = reg_value | ENABLE_FILTER;
    error = writeRegister(DEVICE_ADDRESS, XYZ_DATA_CFG, reg_value);
    return error; 
}

// Disable high pass filtering of the output
byte MMA8452Q_Accelerometer::disableHighPassFilter()
{
    // Register and error code state
    byte reg_value, error;

    // Save the current settings
    error = readRegister(DEVICE_ADDRESS, XYZ_DATA_CFG, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the filter bit low
    reg_value = reg_value & ~ENABLE_FILTER;
    error = writeRegister(DEVICE_ADDRESS, XYZ_DATA_CFG, reg_value);
    return error; 
}

// Set the high pass filter cutoff frequency with an enumerated filter code
byte MMA8452Q_Accelerometer::setRange(range max_range)
{
    // Register and error code state
    byte reg_value, error;

    error = readRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set low noise mode
    if (reg_value == MMA8452Q_Accelerometer::MAX_8G)
	reg_value = reg_value & ~LOW_NOISE;
    else
	reg_value = reg_value | LOW_NOISE;
    error =  writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
	return error;

    // Save the current settings
    error = readRegister(DEVICE_ADDRESS, XYZ_DATA_CFG, reg_value);
    if (error != NO_ERROR)
	return error;

    // Clear the range bits
    reg_value = reg_value & RANGE_MASK;
    // Set the range bits
    reg_value = reg_value | max_range;
    error = writeRegister(DEVICE_ADDRESS, XYZ_DATA_CFG, reg_value);
    return error;
}

// Set the high pass filter cutoff frequency with an enumerated filter code
byte MMA8452Q_Accelerometer::setHighPassCutoff(filter frequency)
{
    // Register and error code state
    byte reg_value, error;

    // Save the current settings
    error = readRegister(DEVICE_ADDRESS, HP_FILTER_CUTOFF, reg_value);
    if (error != NO_ERROR)
	return error;

    // Clear the filter bits
    reg_value = reg_value & FILTER_MASK;
    // Set the filter bits
    reg_value = reg_value | frequency;
    error = writeRegister(DEVICE_ADDRESS, HP_FILTER_CUTOFF, reg_value);
    return error;
}

// Read the acceleration of all three axes and store in memory
byte MMA8452Q_Accelerometer::readData()
{
    // Error code state
    byte error;
    // Buffer for the sensor data
    byte rawData[6];

    // Get acceleration of all 3 axes
    error = readRegisters(DEVICE_ADDRESS, OUT_X_MSB, 6, rawData);
    // Loop through each axis
    for(int i = 0; i < 6 ; i+=2)
    {
    	// Because the data registers hold only 12 bits and they are set so that the first
    	// register can be used on low resource systems the data is offset by 4 bits, shifting
    	// it to the right makes it aligned at the least significant bit
    	data[i+1] = (int8_t)rawData[i] >> 4 ;
    	// Get the low byte
	data[i] = rawData[i] << 4;
    	data[i] |= rawData[i+1] >> 4;
    }

    return error;
}

// Send the data via a specified serial device in byte sized chuncks with the high byte 
// followed by the low byte of each axis, 6 bytes in total
void MMA8452Q_Accelerometer::sendData(HardwareSerial & serial_device, byte delimiter)
{
    for (int i = 0; i < 6; ++i)
    {
	if (data[i] == delimiter)
	    serial_device.write(delimiter);
	serial_device.write(data[i]);
    }
}

// The following enum declarations tie bit codes to enumerations preventing macro collisions
// by defining each bit code as a member of its parent class

// Ranges codes for the acceleration output upper and lower range in gravities
enum range
{
    MAX_2G = RANGE_2G,
    MAX_4G = RANGE_4G,
    MAX_8G = RANGE_8G
};

// Frequency code for the highpass filter cuttoff frequencies
enum filter
{
    CUTOFF_16_HZ = FILTER_16HZ,
    CUTOFF_8_HZ = FILTER_8HZ,
    CUTOFF_4_HZ = FILTER_4HZ,
    CUTOFF_2_HZ = FILTER_2HZ
};


