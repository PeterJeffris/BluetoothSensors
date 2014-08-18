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

// Functions to handle repetitive I2C comunication tasks from an I2C master. The SDA lines and
// SCL lines of all I2C devices can be connected to the master respectively allowing all 
// devices to exist on a single data bus and conserving IO's

// Check to see if these functions have been defined, if not define them
#ifndef I2C_TOOLS
#define I2C_TOOLS

// Libraries containing more basic tools for I2C comunications
#include <Arduino.h>
#include <Wire.h>
#include "stdint.h"

// Read a single byte from a slave device by specifying its address 'deviceAddress', the 
// address of the register to be read 'readAddress', and a reference to the byte to place 
// the result in 'dest'
byte readRegisters(byte deviceAddress, byte readAddress, byte size, byte * dest);

// Read a multiple bytes from a slave device by specifying its address 'deviceAddress', the 
// starting address of the register to be read 'readAddress', the number of bytes to be read
// 'size' and a pointer to the byte array in which the result will be placed 'dest'
byte readRegister(byte deviceAddress, byte readAddress, byte & dest);

// Write a single byte to a slave device by specifying its address 'deviceAdress', the register address to write to 'writeAddress', and a reference to the byte to write 'src'
byte writeRegister(byte deviceAddress, byte writeAddress, const byte & writeData);  

// Write a multiple bytes to a slave device by specifying its address 'deviceAdress',
//the register address to write to 'writeAddress', the number of bytes to be written 'size',
// and a pointer to the byte array to write to 'src'
byte writeRegister(byte deviceAddress, byte writeAddress, byte size, const byte * src);  

#endif
