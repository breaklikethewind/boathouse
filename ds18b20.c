

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "ds18b20.h"
     
int Ds18b20ReadTemp (w1desc dev, float* farenheit, float* celsius) 
{
   char devPath[128]; // Path to device
   char buf[256];     // Data from device
   char tmpData[6];   // Temp C * 1000 reported by device 
   char path[] = "/sys/bus/w1/devices"; 
   ssize_t numRead;
 
   // Assemble path to OneWire device
   sprintf(devPath, "%s/%s/w1_slave", path, dev);
   
   // Opening the device's file triggers new reading
   int fd = open(devPath, O_RDONLY);
   if (fd == -1)
   {
      perror ("Couldn't open the w1 device.");
      return 1;   
   }
      
   while((numRead = read(fd, buf, 256)) > 0) 
   {
      strncpy(tmpData, strstr(buf, "t=") + 2, 5); 
      *celsius = strtof(tmpData, NULL) / 1000;
      *farenheit = *celsius * 9 / 5 + 32;
	   
#ifdef DEBUG
      printf("Device: %s  - ", dev); 
      printf("Temp: %.3f C  ", celsius);
      printf("%.3f F\n\n", farenheit);
#endif
   }
   close(fd);
   
   return 0;
}

int Ds18b20Init(w1desc dev)
{
   DIR *dir;
   struct dirent *dirent;
   char path[] = "/sys/bus/w1/devices"; 
	
   system("modprobe w1-gpio");
   system("modprobe w1-therm");
 
   dir = opendir (path);
   if (dir != NULL)
   {
      while ((dirent = readdir (dir)))
      // 1-wire devices are links beginning with 28-
         if (dirent->d_type == DT_LNK &&  strstr(dirent->d_name, "28-") != NULL) { 
            strcpy(dev, dirent->d_name);
#ifdef DEBUG
            printf("\nDevice: %s\n", dev);
#endif
         }
	 
      (void) closedir (dir);
   }
   else
   {
      perror ("Couldn't open the w1 devices directory");
      return 1;
   }

   return 0;
}


