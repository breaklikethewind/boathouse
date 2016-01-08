
Raspberry PI/RTI monitoring software written by Eric Nelson

This program uses a raspberry pi to take temperature, humidity, and distance
measurements of a sump pit. That data is sent to an RTI XP processor for 
display to the user.

The files are:

beep.c
This is a driver to activate a piezo electric buzzer on the raspberry pi.
This driver can also send morse code using the pi buzzer.

dht_read.c
This is a driver to read a AM2302, or DHT22, temperature/humidity
sensor.

range.c
This is a driver to read an HC-SR04 ultrasonic range module.

ds18b20.c
This is a driver to read temperature from a 1 wire ds18b20 sensor.

tsl2561.c
This is a driver to read the ambient light level from a tsl2561 sensor.

pir.c
This driver reads data from a PIR sensor connected to a GPIO line on the PI.

boathouse.c
This is the main program entry point.

transport.c
This controls the communication to the RTI processor. The communciation
uses the RTI driver "two way strings".

example_system.rti
This is an example of how to setup an XP8s processor to communicate with
the raspberry pi. This system configuration was stripped out of a larger
working system configuration. This file is for documentation purposes only.

Each driver, and transport.c are designed to be self contained re-usable
modules for other programs. 

The drivers init functions identify the pins the sensor is connected to. These
drivers require the wiring pi library be installed.

The transport.c module contains the mechanism to manage the communication
to the RTI processor. Programs can re-use the transport module by defining a
command table (commandlist_t), and a push table (pushlist_t). The command
table defines what variables, or functions, are called when an XP processor request
arrives. The push table defines what periodic data is sent to the processor.

On the RTI processor two way strings driver, you must define the tag strings from
the push list, and the command strings from the tags in the command list. The 
full command list consists of tags defined in your application (boathouse.c in this case),
and tags defined in transport.c.

In the two way strings driver you must define "PAIR", and "SEQUENCENUMBER". These two
strings are used to manage when the PI is connected to the XP8s, and how many
messages have been sent from the PI to the XP8s.

In addition to the two way strings driver, you will want to use the 
"System Variable Events" driver to trigger XP8 macros to run when two way string
variables reach specific limits, or when the PI sends an asynchronous event (such
as a PIR motion event).

This source code makes use of the "WiringPi" library found at:
https://projects.drogon.net/raspberry-pi/wiringpi/


