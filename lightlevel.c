/*
 * lightlevel.c:
 *      This app measures light level using TSL-2561 light sensor module
 *
 *	How to test:
 *      Connect the TSL-2561 module to 3.3V, and I2C port 0 of the 
 *      raspberry pi.
 *
 *      This file originated from Pasquale posted on www.raspberrypi.org
 *      on Jan 29 2014.
 *
 * Copyright (c) 2014 Eric Nelson
 ***********************************************************************
 * This file uses wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <errno.h>

#include "lightlevel.h"

// ALL COMMAND TSL2561
// Default I2C RPI address in (0x39) = FLOAT ADDR (Slave) Other [(0x49) = VCC ADDR / (0x29) = GROUND ADDR]
#define TSL2561_ADDR_LOW                      (0x29)
#define TSL2561_ADDR_FLOAT                    (0x39)   
#define TSL2561_ADDR_HIGH                     (0x49)
#define TSL2561_CONTROL_POWERON               (0x03)
#define TSL2561_CONTROL_POWEROFF              (0x00)
#define TSL2561_GAIN_0X                       (0x00) // No gain
#define TSL2561_GAIN_AUTO                     (0x01)
#define TSL2561_GAIN_1X                       (0x02)
#define TSL2561_GAIN_16X                      (0x12) // (0x10)
#define TSL2561_INTEGRATIONTIME_13MS          (0x00) // 13.7ms
#define TSL2561_INTEGRATIONTIME_101MS         (0x01) // 101ms
#define TSL2561_INTEGRATIONTIME_402MS         (0x02) // 402ms
#define TSL2561_READBIT                       (0x01)
#define TSL2561_COMMAND_BIT                   (0x80) // Must be 1
#define TSL2561_CLEAR_BIT                     (0x40) // Clears any pending interrupt (write 1 to clear)
#define TSL2561_WORD_BIT                      (0x20) // 1 = read/write word (rather than byte)
#define TSL2561_BLOCK_BIT                     (0x10) // 1 = using block read/write
#define TSL2561_REGISTER_CONTROL              (0x00)
#define TSL2561_REGISTER_TIMING               (0x81)
#define TSL2561_REGISTER_THRESHHOLDL_LOW      (0x02)
#define TSL2561_REGISTER_THRESHHOLDL_HIGH     (0x03)
#define TSL2561_REGISTER_THRESHHOLDH_LOW      (0x04)
#define TSL2561_REGISTER_THRESHHOLDH_HIGH     (0x05)
#define TSL2561_REGISTER_INTERRUPT            (0x06)
#define TSL2561_REGISTER_CRC                  (0x08)
#define TSL2561_REGISTER_ID                   (0x0A)
#define TSL2561_REGISTER_CHAN0_LOW            (0x8C)
#define TSL2561_REGISTER_CHAN0_HIGH           (0x8D)
#define TSL2561_REGISTER_CHAN1_LOW            (0x8E)
#define TSL2561_REGISTER_CHAN1_HIGH           (0x8F)

// Delay getLux function
#define LUXDELAY 500

int TSL2561_Addr = TSL2561_ADDR_FLOAT;
int I2C_Port = 1;

int getLux(int fd)
{
   wiringPiI2CWriteReg8(fd, TSL2561_COMMAND_BIT, TSL2561_CONTROL_POWERON); // enable the device
//   wiringPiI2CWriteReg8(fd, TSL2561_REGISTER_TIMING, TSL2561_GAIN_AUTO); // auto gain and timing = 101 mSec
   wiringPiI2CWriteReg8(fd, TSL2561_REGISTER_TIMING, TSL2561_GAIN_16X);    // auto gain and timing = 101 mSec

   // Wait for the conversion to complete
   delay(LUXDELAY);

   // Reads visible + IR diode from the I2C device auto
   uint16_t visible_and_ir = wiringPiI2CReadReg16(fd, TSL2561_REGISTER_CHAN0_LOW);
   wiringPiI2CWriteReg8(fd, TSL2561_COMMAND_BIT, TSL2561_CONTROL_POWEROFF); // disable the device
   
   return visible_and_ir;
}

/*
 *********************************************************************************
 * interface functions
 *********************************************************************************
 */

int LuxInit(int port, int id, int intgpio)
{
   int filedesc;
	
   if (id > 0)
	TSL2561_Addr = id;
   
   if (port >= 0)
	I2C_Port = port;
	
   system("gpio load i2c");
	
   filedesc = wiringPiI2CSetup(TSL2561_Addr);

   return filedesc;
}

int LuxMeasure(int filedesc)
{
   int lux;
	
   lux = getLux(filedesc);
	
   return lux;
}


