#ifndef _R_GPIO_H_
#define _R_GPIO_H_

enum pin_mode 
{
    INPUT,
    OUTPUT,
    ISR_RISING,
    ISR_FALLING,
    ISR_BOTH
};

enum interrupt_type
{
    INT_RISING = 1,
    INT_FALLING = 2
};

int setMode(int pin, int mode);

void digitalWrite(int pin, int value);
int digitalRead(int pin);

int waitInterrupt(int pin);
int attachInterrupt(int pin, int mode, void(*callback)(int));

#endif