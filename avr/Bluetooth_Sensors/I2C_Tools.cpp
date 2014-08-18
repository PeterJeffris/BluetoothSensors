// Writen by Peter Jeffris, University of Colorado Mechanical Engineering

// Functions to handle repetitive I2C comunication tasks from an I2C master. The SDA lines and
// SCL lines of all I2C devices can be connected to the master respectively allowing all 
// devices to exist on a single data bus and conserving IO's

#include "I2C_Tools.h"

// Read a single byte from a slave device by specifying its address 'deviceAddress', the 
// address of the register to be read 'readAddress', and a reference to the byte to place 
// the result in 'dest'
byte readRegister(byte deviceAddress, byte readAddress, byte & dest)
{
    byte error;
    // Open the slave device for writing
    Wire.beginTransmission(deviceAddress);
    // Set the requested register address in the send queue
    Wire.write(readAddress);
    // Send the request register but keep the connection active
    error = Wire.endTransmission(false);
    // Request the byte from the slave device
    Wire.requestFrom(deviceAddress, 1);
    // 
    while(!Wire.available());
    dest = Wire.read();
    return error;
}

// Read a multiple bytes from a slave device by specifying its address 'deviceAddress', the 
// starting address of the register to be read 'readAddress', the number of bytes to be read
// 'size' and a pointer to the byte array in which the result will be placed 'dest'
byte readRegisters(byte deviceAddress, byte readAddress, byte size, byte * dest)
{
    byte error;
    // Open the slave device for writing
    Wire.beginTransmission(deviceAddress);
    // Set the starting address of the registers requested in the send queue
    Wire.write(readAddress);
    // Send the request register but keep the connection alive
    error = Wire.endTransmission(false);
    // Request bytes from the slave starting at the read address until read address + size
    Wire.requestFrom(deviceAddress, size);
    // Get each byte from the recieve queue
    for(int i = 0; i < size; ++i)
    {
	while(!Wire.available()); 
	dest[i] = Wire.read();
    }
    return error;
}

// Write a single byte to a slave device by specifying its address 'deviceAdress', the register address to write to 'writeAddress', and a reference to the byte to write 'src'
byte writeRegister(byte deviceAddress, byte writeAddress, const byte & src)
{
    // Open the slave device for writing
    Wire.beginTransmission(deviceAddress);
    // Write the register address to be writen too
    Wire.write(writeAddress);
    // Write to the register
    Wire.write(src);
    // Clear the send queue and close the connection
    return Wire.endTransmission(); 
}

// Write a multiple bytes to a slave device by specifying its address 'deviceAdress',
//the register address to write to 'writeAddress', the number of bytes to be written 'size',
// and a pointer to the byte array to write to 'src'
byte writeRegisters(byte deviceAddress, byte writeAddress, byte size, const byte * src)
{
    // Open the slave device for writing
    Wire.beginTransmission(deviceAddress);
    // Write the register address to write to
    Wire.write(writeAddress);
    // Write each byte
    for (int i = 0; i < size; ++i) {
	Wire.write(src[i]);
    }
    return Wire.endTransmission();
}
    
