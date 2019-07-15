/*
 * boathouse.c:
 *      This app manages the sump pump
 *
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
 ************************************************************************/
 
/*****************************************************************************/
/* Sensors:                                                                  */
/*    light                                                                  */
/*    PIR                                                                    */
/*    beep                                                                   */
/*    temp & humidity                                                        */
/*    temp -2                                                                */
/*    distance                                                               */
/*                                                                           */
/* Pin Mux:                                                                  */
/*    Name   WPiPin   GPIO   PCB Terminal                                    */
/*    3.3V                        1                                          */
/*    gnd                         2                                          */
/*    1W Temp  7        4         3                                          */
/*    Beep     2        27        4                                          */
/*    SDA Lux  3       pin3       5                                          */
/*    SCL Lux  5       pin5       6                                          */
/*    5.0V                        7                                          */
/*    PIR      3        22        8                                          */
/*    Echo     1        18        9                                          */
/*    Trigger  0        17        10                                         */
/*    3.3V                        11                                         */
/*    gnd                         12                                         */
/*    DHT      5        24        13                                         */
/*    IntLux   6        25        14                                         */
/*                                                                           */
/* Wiring:                                                                   */
/*    temp2  Pin       Wire Color                                            */
/*           VCC       Orange                                                */
/*           GND       Blue                                                  */
/*           Data      Blue/White                                            */
/*    lux                                                                    */
/*           SDA       Orange                                                */
/*           SCL       Green                                                 */
/*           GND       White/Green, White/Orange, White/Brown, White/Blue    */
/*           3V3       Blue                                                  */
/*           Int       Brown                                                 */
/*    PIR                                                                    */
/*           VCC       Red                                                   */
/*           GND       Black                                                 */
/*           Out       Yellow                                                */
/*    Distance                                                               */
/*           VCC       Red                                                   */
/*           Trigger   Yellow                                                */
/*           Echo      Green                                                 */
/*           GND       Black                                                 */
/*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <fcntl.h>
#include <wiringPi.h>
#include <sys/mman.h>
#include <time.h>
#include <arpa/inet.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "beep.h"
#include "range.h"
#include "dht_read.h"
#include "ds18b20.h"
#include "tsl2561.h"
#include "pir.h"
#include "transport.h"

#define BeepPin 2        // Raspberry pi gpio 27
#define EchoPin 1        // Raspberry pi gpio 4
#define TriggerPin 0     // Raspberry pi gpio 17
#define DHTPin 5         // Raspberry pi gpio 24
#define LightIntPin 6    // Raspberry pi gpio 25
#define PIRPin 3         // Raspberry pi gpio 22

#define DEFAULT_SENSOR_PERIOD 60 // Seconds

w1desc GndTempdDev;
TSL2561 light1 = TSL2561_INIT(1, TSL2561_ADDR_FLOAT);

struct sockaddr_in servaddr;

int sockfd;
int rtiUdpPort;
/* This is unique per application instance and RTI driver instance */
#define RTI_UDP_PORT 32002

typedef struct
{
	float humidity_pct;
	float temp_f;
	float distance_in;
	int beeper;
	int light_visable;
	int light_ir;
	int light_lux;
	float gndtemp_f;
	int motion;
	char morse[80];
} status_t;

status_t status;
int sensor_period = DEFAULT_SENSOR_PERIOD;
int exitflag = 0;
int firstsampleflag = 0;
pthread_mutex_t lock; // sync between UDP thread and main
commandlist_t command_list;
void *thread_sensor_sample( void *ptr );
void measure(void);

typedef int (*cmdfunc)(char* request, char* response);

int get_motion(char* request, char* response);
int morse(char* request, char* response); 
int app_exit(char* request, char* response);

pushlist_t pushlist[] = { 
{ "HUMIDITY", TYPE_FLOAT,   &status.humidity_pct}, 
{ "TEMP",     TYPE_FLOAT,   &status.temp_f}, 
{ "DISTANCE", TYPE_FLOAT,   &status.distance_in},
{ "BEEPER",   TYPE_INTEGER, &status.beeper},
{ "LIGHTIR",  TYPE_INTEGER, &status.light_ir},
{ "LIGHTVIS", TYPE_INTEGER, &status.light_visable},
{ "LIGHTLUX", TYPE_INTEGER, &status.light_lux},
{ "GROUNDT",  TYPE_FLOAT,   &status.gndtemp_f},
{ "MOTION",   TYPE_INTEGER, &status.motion},
{ "",         TYPE_NULL,    NULL} 
};

commandlist_t device_commandlist[] = { 
{ "GETHUMIDITY",     "HUMIDITY",     NULL,        TYPE_FLOAT,   &status.humidity_pct}, 
{ "GETTEMP",         "TEMP",         NULL,        TYPE_FLOAT,   &status.temp_f}, 
{ "GETDISTANCE",     "DISTANCE",     NULL,        TYPE_FLOAT,   &status.distance_in},
{ "GETBEEPER",       "BEEPER",       NULL,        TYPE_INTEGER, &status.beeper},
{ "DOMORSE",         "MORSE",        &morse,      TYPE_STRING,  NULL},
{ "GETGROUNDT",      "GROUNDT",      NULL,        TYPE_FLOAT,   &status.gndtemp_f},
{ "GETLUX",          "LIGHTLUX",     NULL,        TYPE_INTEGER, &status.light_lux},
{ "GETVIS",          "LIGHTVIS",     NULL,        TYPE_INTEGER, &status.light_visable},
{ "GETLIR",          "LIGHTIR",      NULL,        TYPE_INTEGER, &status.light_ir},
{ "GETMOTION",       "MOTION",       &get_motion, TYPE_INTEGER, &status.motion},
{ "SETSENSORPERIOD", "SENSORPERIOD", NULL,        TYPE_INTEGER, &sensor_period},
{ "EXIT",            "EXIT",         &app_exit,   TYPE_INTEGER, &exitflag},
{ "",                "",             NULL,        TYPE_NULL,    NULL}
};

void motion_interrupt(void)
{
	status.motion = PIRRead();
	tp_force_data_push();
}

int get_motion(char* request, char* response)
{
	status.motion = PIRRead();
	sprintf(response, "%u", status.motion);
	
	return 0;
}
 
int morse(char* request, char* response) 
{
	BeepMorse(5, request);
	strcpy(response, request);
	
	return 0;
}

int app_exit(char* request, char* response)
{
	char* junk;

	exitflag = strtol(request, &junk, 0);
	sprintf(response, "%u", exitflag);
	
	return 0;
}

void *thread_sensor_sample( void *ptr ) 
{
	
	while (!exitflag)
	{
		measure();
		sleep(sensor_period);
	}
	
	return NULL;
}

void measure( void )
{
	float temp_c;
	uint16_t visable;
	uint16_t ir;
	uint32_t lux;

	// Fetch sensor data
	pthread_mutex_lock(&lock);
	Ds18b20ReadTemp(GndTempdDev, &(status.gndtemp_f), &temp_c);
	pthread_mutex_unlock(&lock);
	
	pthread_mutex_lock(&lock);
	// sense the luminosity from the sensor (lux is the luminosity taken in "lux" measure units)
	// the last parameter can be 1 to enable library auto gain, or 0 to disable it
	TSL2561_SENSELIGHT(&light1, &visable, &ir, &lux, 1);
	status.light_visable = visable;
	status.light_ir = ir;
	status.light_lux = lux;
	pthread_mutex_unlock(&lock);
		
	pthread_mutex_lock(&lock);
	dht_read_val(&(status.temp_f), &temp_c, &(status.humidity_pct));
	pthread_mutex_unlock(&lock);
		
	pthread_mutex_lock(&lock);
	status.distance_in = RangeMeasure(5, status.temp_f);
	pthread_mutex_unlock(&lock);
	
	pthread_mutex_lock(&lock);
	status.motion = PIRRead();
	
	firstsampleflag = 1;
	pthread_mutex_unlock(&lock);
}

int init_tsl2561(void)
{
	int rc;

	// Load I2C kernel drivers
	system("modprobe i2c_bcm2708");
   	system("modprobe i2c_dev");

	// prepare the sensor
	// (the first parameter is the raspberry pi i2c master controller attached to the TSL2561, the second is the i2c selection jumper)
	// The i2c selection address can be one of: TSL2561_ADDR_LOW, TSL2561_ADDR_FLOAT or TSL2561_ADDR_HIGH
//	TSL2561 light1 = TSL2561_INIT(1, TSL2561_ADDR_FLOAT);
	
	// initialize the sensor
	rc = TSL2561_OPEN(&light1);
	if (rc != 0) 
	{
		fprintf(stderr, "Error initializing TSL2561 sensor (%s). Check your i2c bus (es. i2cdetect)\n", strerror(light1.lasterr));
		// you don't need to TSL2561_CLOSE() if TSL2561_OPEN() failed, but it's safe doing it.
		TSL2561_CLOSE(&light1);
		return 1;
	}
	
	// set the gain to 1X (it can be TSL2561_GAIN_1X or TSL2561_GAIN_16X)
	// use 16X gain to get more precision in dark ambients, or enable auto gain below
	rc = TSL2561_SETGAIN(&light1, TSL2561_GAIN_1X);
	
	// set the integration time 
	// (TSL2561_INTEGRATIONTIME_402MS or TSL2561_INTEGRATIONTIME_101MS or TSL2561_INTEGRATIONTIME_13MS)
	// TSL2561_INTEGRATIONTIME_402MS is slower but more precise, TSL2561_INTEGRATIONTIME_13MS is very fast but not so precise
	rc = TSL2561_SETINTEGRATIONTIME(&light1, TSL2561_INTEGRATIONTIME_101MS);

	return rc;
}

/*
 *********************************************************************************
 * main
 *********************************************************************************
 */

int  main(void)
{
	int  iret1;
	int broadcast;
	pthread_t sensor_sample;

	printf("Boathouse Launch...\r\n");
	// Setup GPIO's, Timers, Interrupts, etc
	if (wiringPiSetup() == -1)
		exit(1);
	/* Set up the socket */
	rtiUdpPort = RTI_UDP_PORT;
	broadcast = 1;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(rtiUdpPort);
	bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	// Initialize sensors
	BeepInit(BeepPin, 0);
	RangeInit(EchoPin, TriggerPin, 0);
	dht_init(DHTPin);
	Ds18b20Init(GndTempdDev);
	init_tsl2561();
	PIRInit(PIRPin);

	PIRRegisterCallback(&motion_interrupt);
	
	iret1 = pthread_mutex_init(&lock, NULL); 
	if(iret1)
	{
		BeepMorse(5, "Mutex Fail");
		printf("Error - mutex init failed, return code: %d\n",iret1);
		return -1;
	}

	/* Initialize the threads */
	iret1 = pthread_create( &sensor_sample, NULL, thread_sensor_sample, NULL);
	if(iret1)
	{
		printf("Error - pthread_create() return code: %d\n",iret1);
		BeepMorse(5, "thread_sensor_sample Thread Create Fail");
		return -2;
	}
	else
		printf("Launching thread sensor_sample\r\n");

	// wait until we have our first sample
	while(!firstsampleflag);
	
	tp_handle_requests(device_commandlist, &lock);
	
	tp_handle_data_push(pushlist, &lock);

	BeepMorse(5, "OK");
	
	//while (!exitflag) sleep(0);
	tp_join();
	exitflag = 1;

	printf("Boathouse Exit Set...\r\n");
	
	// Exit	
	tp_stop_handlers();
	pthread_join(sensor_sample, NULL);
	pthread_mutex_destroy(&lock);
	TSL2561_CLOSE(&light1);

	BeepMorse(5, "Exit");
	
	while(1);
	
	return 0;
}

