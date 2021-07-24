// [Working]
// GPIO IN - GPIO6 
// GPIO OUT - GPIO21

// install
//  sudo apt-get install gpiod libgpiod-dev libgpiod-doc

// compile
//	gcc gpio_test.c -o gpio_test -lrduino

#include <stdlib.h>
#include <stdio.h> 
#include <unistd.h>
#include <fcntl.h>  //define O_WRONLY and O_RDONLY 
#include <poll.h>
#include <gpiod.h>

#include <r_gpio.h>


void callback_function()
{
    int edge = digitalRead(6);

    printf("Interrupt ");
    switch(edge)
    {
        case 0:
            printf("Falling!\n");
            break;
        case 1:
            printf("Rising!\n");
            break;
    }
}

int main()
{   
    setMode(21,OUTPUT);
    attachInterrupt(6, ISR_BOTH, &callback_function);

    while(1)
     {
        digitalWrite(21, 1);
        sleep(1);

        digitalWrite(21, 0);
        sleep(1);
    }
}