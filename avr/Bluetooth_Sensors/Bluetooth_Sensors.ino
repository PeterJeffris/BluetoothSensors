// Writen by Peter Jeffris, University of Colorado Mechanical Engineering
// Module to collect data from various sensors and respond to bluetooth
// requests for that data.

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

#define DEBUG 0

// Include sensor comunication and configuration libraries 
#include "MMA8452Q_Accelerometer.h"
#include "MPL3115A2_Barometer.h"
#include "L3G4200D_Gyroscope.h"

// Include I2C Library
#include "Wire.h"

// Comunication codes
enum network
{
    START_STREAM = 0xB0,
    END_STREAM = 0xB1,
    SEND_SINGLE = 0xB2,
    DLE = 0x10,
    STX = 0x20,
    ETX = 0x30,
    ACC = 0x01,
    GYRO = 0x02,
    BARO = 0x04,
    PHT = 0x08
};

// Error handeling codes
#define NO_ERROR 0
#define BUFFER_SIZE_ERROR 1
#define ADDRESS_NO_ACKNOWLEDGE 2
#define DATA_NO_ACKNOWLEDGE 3
#define TWI_ERROR 4
#define IDENTIFICATION_FAILURE 5

// Initialize the sensor objects
MMA8452Q_Accelerometer accelerometer;
L3G4200D_Gyroscope gyrometer;
MPL3115A2_Barometer barometer;
unsigned int ambient_light;
#define PHOTO_SENSOR_PIN A3

// Request code byte to be recieved by the 
char request;
// Error code resulting from I2C comunication
byte error;

// Timing variables
unsigned int start,stop,diff;
// Downsampling counter for the barometric sensor
short int sample_count;

// Initialize the sensors and serial objects
void setup()
{
    // Setup a hardware serial connection
    Serial.begin(115000);
    // Serial.print("Started serial\n");
    // Wire.begin();
    // Serial.print("Started i2c\n");
    // byte reg_value;
    // byte error = readRegister(0x60, 0x0C, reg_value);
    // Serial.print("Read identity\n");
    // Serial.print(reg_value,HEX);
#if DEBUG
    Serial.print("Starting setup\n");
#endif
    // Initialize the sensors on the I2C bus
    error = accelerometer.setup();
    if (error != NO_ERROR)
    {
#if DEBUG
	Serial.print("Accelerometer setup error: ");
	Serial.print(error);
	Serial.println();
#else
    while(true) {}
#endif
    }
#if DEBUG
    else
	Serial.print("Setup Accelerometer\n");
#endif

    error = gyrometer.setup();
    if (error != NO_ERROR)
    {
#if DEBUG
	Serial.print("Gyrometer setup error: ");
	Serial.print(error);
	Serial.println();
#else
	while(true) {}
#endif
    }
#if DEBUG
    else
	Serial.print("Setup Gyrometer\n");
#endif

    error = barometer.setup();
    if (error != NO_ERROR)
    {
#if DEBUG
	Serial.print("Barometer setup error: ");
	Serial.print(error);
	Serial.println();
#else
	while(true) {}
#endif
    }
#if DEBUG
    else
	Serial.print("Setup Barometer\n");
#endif
    start = millis();
}

void getData()
{
#if DEBUG
    error = accelerometer.readData();
    if (error != NO_ERROR)
    {
	Serial.print("Accelerometer communication error: ");
	Serial.print(error);
	Serial.println();
    }
    error = gyrometer.readData();
    if (error != NO_ERROR)
    {
	Serial.print("Gyro communication error: ");
	Serial.print(error);
	Serial.println();
    }
    if (sample_count == 0) {
	error = barometer.readData();
	if (error != NO_ERROR)
	{
	    Serial.print("Altimeter communication error: ");
	    Serial.print(error);
	    Serial.println();
	}
	ambient_light = analogRead(PHOTO_SENSOR_PIN);
    }
#else
    accelerometer.readData();
    gyrometer.readData();
    if (sample_count == 0) {
	barometer.readData();
	ambient_light = analogRead(PHOTO_SENSOR_PIN);
    }
#endif
}



void print_data() {
    getData();
    ambient_light = analogRead(PHOTO_SENSOR_PIN);
    stop = micros();
    diff = stop-start;
    start = stop;
    Serial.print(accelerometer.acc[0]);
    Serial.print(',');
    Serial.print(accelerometer.acc[1]);
    Serial.print(',');
    Serial.print(accelerometer.acc[2]);
    Serial.print(',');
    Serial.print(gyrometer.gyro[0]);
    Serial.print(',');
    Serial.print(gyrometer.gyro[1]);
    Serial.print(',');
    Serial.print(gyrometer.gyro[2]);
    if (sample_count == 0) {
	Serial.print(',');
	Serial.print((float)barometer.pressure+(float)barometer.pressure_frac/16);
	Serial.print(',');
	Serial.print((float)barometer.temperature+(float)barometer.temperature_frac/16);
	Serial.print(',');
	Serial.print(ambient_light);
    }
    Serial.print(',');
    Serial.print(diff);
    Serial.print('\n');
}

// Send sensor packets via bluetooth
void bluetooth_send() {
    getData();
    stop = micros();
    diff = stop-start;
    start = stop;
    Serial.write(DLE);
    Serial.write(STX);
    if (highByte(diff) == DLE)
	Serial.write(DLE);
    Serial.write(highByte(diff));
    if (lowByte(diff) == DLE)
	Serial.write(DLE);
    Serial.write(lowByte(diff));

    Serial.write(DLE);
    Serial.write(ACC);
    accelerometer.sendData(Serial,DLE);

    Serial.write(DLE);
    Serial.write(GYRO);
    gyrometer.sendData(Serial,DLE);

    if (sample_count == 0) {
	Serial.write(DLE);
	Serial.write(BARO);
	barometer.sendData(Serial,DLE);

	Serial.write(DLE);
	Serial.write(PHT);
	if (lowByte(ambient_light) == DLE)
	    Serial.write(DLE);
	Serial.write(lowByte(ambient_light));
	if (highByte(ambient_light) == DLE)
	    Serial.write(DLE);
	Serial.write(highByte(ambient_light));
    }
    Serial.write(DLE);
    Serial.write(ETX);
}

void loop() {
    sample_count = 0;
    start = micros();
    for (;;) {
	// While data request continue to come in serve them as fast as possible
	// by getting data ready while the other end is working
	char request = Serial.read();
	if (request == START_STREAM) {
	    while (Serial.read() != END_STREAM) {
		if (sample_count == 100)
		    sample_count = 0;
#if DEBUG
		print_data();
#else
		bluetooth_send();
#endif
		sample_count += 1;
	    }
	}
	else if (request == SEND_SINGLE) {
#if DEBUG
	    print_data();
#else
	    bluetooth_send();
#endif
	}
    }
}	    

