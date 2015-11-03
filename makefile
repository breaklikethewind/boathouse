
CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-lwiringPi -lpthread
SOURCES=boathouse.c beep.c dht_read.c range.c transport.c tsl2561.c pir.c ds18b20.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=boathouse

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
