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

// Class to provide an interface for the L3G4200D MEMS gyroscope rate sensor IC. The sensor
// hosts high and low pass filtering functions, power management and variable range. Output
// reflects the angular velocity or angular rate of the motion around its central axis.
// This device comunicates on I2C and can be attached to a common data bus with other
// inertial sensors.

// Compiler directive to make sure the class has not already been defined
#ifndef L3G4200D_GYROSCOPE
#define L3G4200D_GYROSCOPE

// Headers for other tools used within the class
#include "Arduino.h"
#include "Wire.h"
#include "I2C_Tools.h"
#include "HardwareSerial.h"
#include "stdint.h"

class L3G4200D_Gyroscope {
// Internal members not used outside the class
private:
    byte standby();
    byte activate();
    byte reset();

// Member functions and enumerations accesible outside the class
public:
    union
    {
	int gyro[3];
	char data[6];
    };

    // Ranges codes for the gyrometer rate output upper and lower range in degrees per second
    enum range
    {
      MAX_200_DPS,
      MAX_500_DPS,
      MAX_2000_DPS 
    };

    // Bandwidth code for difference between the highpass and lowpass filter cutoff frequencies
    enum bandwidth
    {
      BAND_110_HZ,
      BAND_50_HZ,
      BAND_35_HZ,
      BAND_30_HZ
    };

    // Frequency code for the highpass filter cuttoff frequencies
    enum filter
    {
      CUTOFF_56_HZ,
      CUTOFF_30_HZ,
      CUTOFF_15_HZ,
      CUTOFF_8_HZ,
      CUTOFF_4_HZ,
      CUTOFF_2_HZ,
      CUTOFF_1_HZ,
      CUTOFF_HALF_HZ,
      CUTOFF_FIFTH_HZ,
      CUTOFF_TENTH_HZ
    };

    // Initialization of the communication and sensor hardware
    byte setup();

    // Set the upper and lower range to be measured in degrees per second defined by a range
    // code enumeration
    byte setRange(range max_range);

    // Enable mode to turn off the sampling hardware power if no requests are
    // recived within a short timeout period
    byte enableSleep();

    // Disable sleep mode for responsive sampling at the expense of incresed power consumption
    byte disableSleep();

    // Enable high pass filtering on the output
    byte enableHighPassFilter();

    // Disable high pass filtering of the output
    byte disableHighPassFilter();

    // Set the high pass filter cutoff frequency with an enumerated filter code
    byte setHighPassCutoff(filter frequency); 

    // Enable a low pass filter on the output
    byte enableLowPassFilter();

    // Disable a low pass filter on the output
    byte disableLowPassFilter();

    // Set a lowpass filter cutoff relative to the high pass filter cutoff with an enumerated
    // bandwidth code, the bandwidth is the difference in these cutoffs
    byte setLowPassBandwidth(bandwidth band);

    // Read the rotational rate of all three axes and store in memory
    byte readData();

    // Send the data via a specified serial device in byte sized chuncks with the high byte 
    // followed by the low byte of each axis, 6 bytes in total
    void sendData(HardwareSerial & serial_device, byte delimiter);
};

#endif
