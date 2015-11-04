


#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sched.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "pir.h"

int pirpin = -1;

int PIRRegisterCallback(pirfunc func)
{
	int retval;
	
	if ( func && (pirpin > 0) )
	{
		wiringPiISR(pirpin, INT_EDGE_BOTH, func);
		retval = 0;
	}
	else // NULL
		retval = -1;
	
	return retval;
}

int PIRRead(void)
{
	int pirpin_state;
	
	pirpin_state = digitalRead(pirpin);
	
	return pirpin_state;
}

int PIRInit(int pin)
{
	pirpin = pin;
	
	pinMode(pirpin, INPUT); // set as high impedance

	return 0;
}

