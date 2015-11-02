


#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sched.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "pir.h"

int pirpin;
//pirfunc int* callback;

//void PIRInterrupt(void)
//{
//	state = PIRRead();
//}

#if 0
int PIRRegisterCallback(pirfunc* func)
{
	if (func)
		wiringPiISR (pirpin, INT_EDGE_BOTH, &PIRInterrupt);
	else // NULL
		pinMode(pirpin, INPUT);

}
#endif

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

