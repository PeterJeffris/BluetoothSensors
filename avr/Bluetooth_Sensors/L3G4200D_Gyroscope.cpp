// Writen by Peter Jeffris, University of Colorado Mechanical Engineering

// Class to provide an interface for the L3G4200D MEMS gyroscope rate sensor IC. The sensor
// hosts high and low pass filtering functions, power management and variable range. Output
// reflects the angular velocity or angular rate of the motion around its central axis.
// This device comunicates on I2C and can be attached to a common data bus with other
// inertial sensors.

#include "L3G4200D_Gyroscope.h"

// I2C address defined by the datasheet and jumper configuration
#define DEVICE_ADDRESS 0x69

// Register addresses from ST Datasheets
#define WHO_AM_I   0x0F
#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define CTRL_REG4 0x23
#define CTRL_REG5 0x24
#define REFERENCE 0x25
#define OUT_TEMP 0x26
#define STATUS_REG 0x27
#define OUT_X_L 0x28
#define OUT_X_H 0x29
#define OUT_Y_L 0x2A
#define OUT_Y_H 0x2B
#define OUT_Z_L 0x2C
#define OUT_Z_H 0x2D
#define FIFO_CTRL_REG 0x2E
#define FIFO_SRC_REG 0x2F

//Register values from ST datasheets
#define WHO_AM_I_VALUE 0xD3
#define ACTIVE 0x08
#define DISABLE_SLEEP 0x07
#define OUTPUT_800HZ 0xC0
#define OUTPUT_MASK 0x3F
#define BANDWIDTH_110HZ 0x30
#define BANDWIDTH_50HZ 0x20
#define BANDWIDTH_35HZ 0x10
#define BANDWIDTH_30HZ 0x00
#define BANDWIDTH_MASK 0xCF
#define FILTER_56HZ 0x00
#define FILTER_30HZ 0x01
#define FILTER_15HZ 0x02
#define FILTER_8HZ 0x03
#define FILTER_4HZ 0x04
#define FILTER_2HZ 0x05
#define FILTER_1HZ 0x06
#define FILTER_HALFHZ 0x07
#define FILTER_FIFTHHZ 0x08
#define FILTER_TENTHHZ 0x09
#define FILTER_MASK 0xF0
#define REBOOT 0x80
#define ENABLE_HP_FILTER 0x11
#define ENABLE_LP_FILTER 0x02
#define RANGE_200DPS 0x00
#define RANGE_500DPS 0x10
#define RANGE_2000DPS 0x20
#define RANGE_MASK 0x60


// Error handeling codes
#define NO_ERROR 0
#define BUFFER_SIZE_ERROR 1
#define ADDRESS_NO_ACKNOWLEDGE 2
#define DATA_NO_ACKNOWLEDGE 3
#define TWI_ERROR 4
#define IDENTIFICATION_FAILURE 5


// Initialization of the communication and sensor hardware
byte L3G4200D_Gyroscope::setup()
{
    // Setup the I2C library
    Wire.begin();

    // Register and error code state
    byte reg_value, error;

    // Check the identity register
    error = readRegister(DEVICE_ADDRESS, WHO_AM_I, reg_value);
    if (error != NO_ERROR)
	return error;

    // Stop if the identity for this address is incorrect
    if (reg_value != WHO_AM_I_VALUE)
    {
	error = IDENTIFICATION_FAILURE;
	return error;
    }

    // Power down the sampling hardware
    error = standby();
    if (error != NO_ERROR)
    	return error;

    // Reset all registers to default
    error = reset();
    if (error != NO_ERROR)
    	return error;

    // Get the main control state
    error = readRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
    	return error;

    // Initialize with maximum sample rate
    reg_value = reg_value & OUTPUT_MASK;
    reg_value = reg_value | OUTPUT_800HZ;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
    	return error;

    // Activate sampling
    error = activate();
    return error;
}

// Activate sampling hardware amplifiers
byte L3G4200D_Gyroscope::activate()
{
    // Register and error code state
    byte reg_value, error;

    // Get control register state so settings are saved
    error = readRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
	return error;

    // Flip the active state bit
    reg_value = reg_value | ACTIVE;
    error =  writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    return error;
}

// Turn off the sampling hardware
byte L3G4200D_Gyroscope::standby()
{
    // Register and error code state
    byte reg_value, error;

    // Get control register state to save setttings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
	return error;

    // Send the active bit low
    reg_value = reg_value & ~ACTIVE;
    return writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value); 
}

// Reset all settings and data values
byte L3G4200D_Gyroscope::reset()
{
    // Register and error code state
    byte reg_value, error;

    // Set the reboot flag high, it will automatically clear
    reg_value = REBOOT;
    error =  writeRegister(DEVICE_ADDRESS, CTRL_REG5, reg_value);
    return error;
}

// Set the upper and lower range to be measured in degrees per second defined by a range
// code enumeration
byte L3G4200D_Gyroscope::setRange(range max_range)
{
    // Register and error code state
    byte reg_value, error;

    // Get the current settings state
    error = readRegister(DEVICE_ADDRESS, CTRL_REG4, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the range bits based in the masked range
    reg_value = reg_value & RANGE_MASK;
    reg_value = reg_value | max_range;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG4, reg_value);
    return error;
}

// Enable mode to turn off the sampling hardware power if no requests are
// recived within a short timeout period
byte L3G4200D_Gyroscope::enableSleep()
{
    // Register and error code state
    byte reg_value, error;

    // Get the current state of the control register to save the settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the sleep bit low
    reg_value = reg_value & ~DISABLE_SLEEP; 
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value); 
    return error; 
}

// Disable sleep mode for responsive sampling at the expense of incresed power consumption
byte L3G4200D_Gyroscope::disableSleep()
{
    // Register and error code state
    byte reg_value, error;

    // Get the current settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the sleep bit high
    reg_value = reg_value | DISABLE_SLEEP;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    return error;
}

// Enable high pass filtering on the output
byte L3G4200D_Gyroscope::enableHighPassFilter()
{
    // Register and error code state
    byte reg_value, error;

    // Save the current register values
    error = readRegister(DEVICE_ADDRESS, CTRL_REG5, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the high pass flag high
    reg_value = reg_value | ENABLE_HP_FILTER;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG5, reg_value);
    return error; 
}

// Disable high pass filtering of the output
byte L3G4200D_Gyroscope::disableHighPassFilter()
{
    // Register and error code state
    byte reg_value, error;

    // Save the previous register states
    error = readRegister(DEVICE_ADDRESS, CTRL_REG5, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the high pass flag low
    reg_value = reg_value & ~ENABLE_HP_FILTER;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG5, reg_value);
    return error; 
}

// Set the high pass filter cutoff frequency with a filter enumerated code
byte L3G4200D_Gyroscope::setHighPassCutoff(filter frequency)
{
    // Register and error code state
    byte reg_value, error;

    // Save the previous register states
    error = readRegister(DEVICE_ADDRESS, CTRL_REG2, reg_value);
    if (error != NO_ERROR)
	return error;

    // Wipe the frequency code bits
    reg_value = reg_value & FILTER_MASK;
    // Update the new code in the register
    reg_value = reg_value | frequency;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG2, reg_value);
    return error;
}

// Enable a low pass filter on the output
byte L3G4200D_Gyroscope::enableLowPassFilter()
{
    // Register and error code state
    byte reg_value, error;

    // Save the other settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG5, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the lowpass filter bit high
    reg_value = reg_value | ENABLE_LP_FILTER;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG5, reg_value);
    return error; 
}

// Disable a low pass filter on the output
byte L3G4200D_Gyroscope::disableLowPassFilter()
{
    // Register and error code state
    byte reg_value, error;

    // Save the other settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG5, reg_value);
    if (error != NO_ERROR)
	return error;

    // Set the low pass filter low
    reg_value = reg_value & ~ENABLE_LP_FILTER;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG5, reg_value);
    return error; 
}

// Set a lowpass filter cutoff relative to the high pass filter cutoff with an enumerated
// bandwidth code, the bandwidth is the difference in these cutoffs
byte L3G4200D_Gyroscope::setLowPassBandwidth(bandwidth band)
{
    // Register and error code state
    byte reg_value, error;

    // Save the other settings
    error = readRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    if (error != NO_ERROR)
	return error;

    // Clear the bandwidth bits
    reg_value = reg_value & BANDWIDTH_MASK;
    // Set the bandwidth code
    reg_value = reg_value | band;
    error = writeRegister(DEVICE_ADDRESS, CTRL_REG1, reg_value);
    return error;
}

// Read the rotational rate off all three axes and store in memory
byte L3G4200D_Gyroscope::readData()
{
    // Register and error code state
    byte error, reg_value;
    // Loop through each axis using sequential byte offsets from the first axis high byte
    // The I2C implimentation is not compatible with Wirings buffered readRegister() function
    for(int i = 0; i < 6 ; i++)
    {
	error = readRegister(DEVICE_ADDRESS, OUT_X_L + i , reg_value);
	if (error != 0)
	    return error;

    	data[i] = reg_value;
    }
    return error;
}

// Send the data via a specified serial device in byte sized chuncks with the high byte 
// followed by the low byte of each axis, 6 bytes in total
void L3G4200D_Gyroscope::sendData(HardwareSerial & serial_device, byte delimiter)
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

// Ranges for the gyrometer rate output
enum range
{
    MAX_200_DPS = RANGE_200DPS,
    MAX_500_DPS = RANGE_500DPS,
    MAX_2000_DPS = RANGE_2000DPS
};

// Difference between the highpass and lowpass filter cutoff frequencies
enum bandwidth
{
    BAND_110_HZ = BANDWIDTH_110HZ,
    BAND_50_HZ = BANDWIDTH_50HZ,
    BAND_35_HZ = BANDWIDTH_35HZ,
    BAND_30_HZ = BANDWIDTH_30HZ
};

// The highpass filter cuttoff frequencies
enum filter
{
    CUTOFF_56_HZ = FILTER_56HZ,
    CUTOFF_30_HZ = FILTER_30HZ,
    CUTOFF_15_HZ = FILTER_15HZ,
    CUTOFF_8_HZ = FILTER_8HZ,
    CUTOFF_4_HZ = FILTER_4HZ,
    CUTOFF_2_HZ = FILTER_2HZ,
    CUTOFF_1_HZ = FILTER_1HZ,
    CUTOFF_HALF_HZ = FILTER_HALFHZ,
    CUTOFF_FIFTH_HZ = FILTER_FIFTHHZ,
    CUTOFF_TENTH_HZ = FILTER_TENTHHZ
};
