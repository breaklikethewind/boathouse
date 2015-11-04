
#ifndef PIR_H
#define PIR_H

typedef void (*pirfunc)(void);
 
int PIRInit(int pin);
int PIRRead(void);
int PIRRegisterCallback(pirfunc func);

#endif // #ifndef PIR_H

