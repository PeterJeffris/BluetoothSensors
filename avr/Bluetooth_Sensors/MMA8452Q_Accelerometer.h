// Writen by Peter Jeffris, University of Colorado Mechanical Engineering

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// Class to provide an interface to the MMA8452Q MEMS IC. This sensor provides acceleration values in three axes with dynamic filtering, range settings and power saving modes.

// Compiler directive to make sure the class has not already been defined
#ifndef MMA8452Q_ACCELEROMETER
#define MMA8452Q_ACCELEROMETER

// Headers for other tools used within the class
#include "Arduino.h"
#include "Wire.h"
#include "I2C_Tools.h"
#include "HardwareSerial.h"
#include "stdint.h"

#define RANGE_2G 0x00
#define RANGE_4G 0x01
#define RANGE_8G 0x02

#define FILTER_16HZ 0x00
#define FILTER_8HZ 0x01
#define FILTER_4HZ 0x02
#define FILTER_2HZ 0x03

class MMA8452Q_Accelerometer {
// Internal members not used outside the class
private:
    byte standby();
    byte resume();
    byte reset();
	
// Member functions and enumerations accesible outside the class
public:
    union
    {
	int acc[3];
	byte data[6];
    };

    // Ranges codes for the acceleration output upper and lower range in gravities
    enum range
    {
      MAX_2G,
      MAX_4G,
      MAX_8G
    };

    // Frequency code for the highpass filter cuttoff frequencies
    enum filter
    {
      CUTOFF_16_HZ,
      CUTOFF_8_HZ,
      CUTOFF_4_HZ,
      CUTOFF_2_HZ
    };

    // Initialization of the communication and sensor hardware
    byte setup();

    // Set the upper and lower range to be measured in degrees per second defined by a range
    // code enumeration
    byte setRange(range max_range);

    // Enable mode to turn off the sampling hardware power if no requests are
    // recived within a short timeout period
    byte enableSleepOnInactivity();

    // Disable sleep mode for responsive sampling at the expense of incresed power consumption
    byte disableSleepOnInactivity();

    // Enable high pass filtering on the output
    byte enableHighPassFilter();
    
    // Disable high pass filtering of the output
    byte disableHighPassFilter();

    // Set the high pass filter cutoff frequency with an enumerated filter code
    byte setHighPassCutoff(filter frequency); 

    // Read the acceleration of all three axes and store in memory
    byte readData();

    // Send the data via a specified serial device in byte sized chuncks with the high byte 
    // followed by the low byte of each axis, 6 bytes in total
    void sendData(HardwareSerial & serial_device, byte delimiter);
};

#endif
