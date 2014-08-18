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

// Class to provide and interface for the MPL3115A2 barometric pressure and altitude sensor. 
// The sensor provides conversion from altitude to pressure.

// Compiler directive to make sure the class has not already been defined
#ifndef MPL3115A2_BAROMETER
#define MPL3115A2_BAROMETER

// Headers for other tools used within the class
#include "Arduino.h"
#include "Wire.h"
#include "I2C_Tools.h"
#include "stdint.h"


class MPL3115A2_Barometer{
// Internal members not used outside the class
private:
    uint8_t standby();
    uint8_t resume();
    uint8_t reset();

// Member functions and enumerations accesible outside the class
public:
    union
    {
	struct
	{
	    int16_t pressure;
	    uint8_t pressure_frac;
	    int8_t temperature;
	    uint8_t temperature_frac;
	};
	char data[6];
    };
    
    // Initialization of the communication and sensor hardware
    uint8_t setup();

    // Read and store the altitude and temperature data
    uint8_t readData();

    // Send the altitude and temperature data with the specified serial device in byte sized
    // chuncks with a format of altitude integer high and low bytes, altitude fractional byte
    // termperature integer byte and temperature fractional byte (the fractional bytes are in
    // 4 bit fixed point format)
    void sendData(HardwareSerial & serial_device, byte delimiter);
};

#endif
