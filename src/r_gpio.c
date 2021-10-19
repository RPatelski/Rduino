#include <pthread.h>
#include <gpiod.h>

#include "r_gpio.h"

#define RGPIO_CONSUMER "r_gpio"
#define RGPIO_COUNT 58

//handle to RPi GPIO chip, opened only once at initialization
struct gpiod_chip* rpi_chip = NULL;

//info about all GPIOs
struct gpio_info
{
    struct gpiod_line* line;
    int interrupt;
};

//array of informations about all GPIOs
struct gpio_info gpios[RGPIO_COUNT];

void tryInitGpio()
{
    if(rpi_chip == NULL)
        rpi_chip = gpiod_chip_open_by_name("gpiochip0");
}

int setMode(int pin, int mode)
{
    tryInitGpio();

    if(pin < 0) return -1;

    gpios[pin].line = gpiod_chip_get_line(rpi_chip, pin);
    gpiod_line_release(gpios[pin].line);

    detachInterrupt(pin);

    switch(mode)
    {
        case OUTPUT:
            return gpiod_line_request_output(gpios[pin].line, RGPIO_CONSUMER, 0);

        case INPUT:
            return gpiod_line_request_input(gpios[pin].line, RGPIO_CONSUMER);

        case ISR_RISING:
            return gpiod_line_request_rising_edge_events(gpios[pin].line, RGPIO_CONSUMER);

        case ISR_FALLING:
            return gpiod_line_request_falling_edge_events(gpios[pin].line, RGPIO_CONSUMER);

        case ISR_BOTH:
            return gpiod_line_request_both_edges_events(gpios[pin].line, RGPIO_CONSUMER);
            
        default:
            return -1;
    }
}

void digitalWrite(int pin, int value)
{
    if(pin < 0) return;

    gpiod_line_set_value(gpios[pin].line, value);
    return;
}

int digitalRead(int pin)
{
    if(pin < 0) return -1;

    return gpiod_line_get_value(gpios[pin].line);
}

int waitInterrupt(int pin)
{
    if(pin < 0) return -1;

    int ret;
    ret = gpiod_line_event_wait(gpios[pin].line, NULL);
    if(ret < 0) return ret;

    struct gpiod_line_event event;
    ret =  gpiod_line_event_read(gpios[pin].line, &event);

    return event.event_type;
}

struct interruptAttr_gpio
{
    int pin;
    void(*callback)();
    void* params;
    volatile bool copied;
};

void* interruptThreadArg_gpio(void* attr)
{
    int pin = ((struct interruptAttr_gpio*)attr)->pin;
    void(*callback)(void*) = ((struct interruptAttr_gpio*)attr)->callback;
    void* params = ((struct interruptAttr_gpio*)attr)->params;
    ((struct interruptAttr_gpio*)attr)->copied = true;

    while(1)
    {
        int edge = waitInterrupt(pin);
        if(edge > 0)
            callback(params);
    }

    return NULL;
}

void* interruptThread_gpio(void* attr)
{
    int pin = ((struct interruptAttr_gpio*)attr)->pin;
    void(*callback)() = ((struct interruptAttr_gpio*)attr)->callback;
    ((struct interruptAttr_gpio*)attr)->copied = true;

    while(1)
    {
        int edge = waitInterrupt(pin);
        if(edge > 0)
            callback();
    }

    return NULL;
}

int attachInterruptArg(int pin, int mode, void(*callback)(void*), void* params)
{
    tryInitGpio();
    
    if(pin < 0) return -1;

    gpios[pin].line = gpiod_chip_get_line(rpi_chip, pin);
    gpiod_line_release(gpios[pin].line);

    detachInterrupt(pin);

    int ret;
    switch(mode)
    {
        case ISR_RISING:
            ret = gpiod_line_request_rising_edge_events(gpios[pin].line, RGPIO_CONSUMER);
            break;
        case ISR_FALLING:
            ret = gpiod_line_request_falling_edge_events(gpios[pin].line, RGPIO_CONSUMER);
            break;
        case ISR_BOTH:
            ret = gpiod_line_request_both_edges_events(gpios[pin].line, RGPIO_CONSUMER);
            break;
    }


    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    struct sched_param shparam = {sched_get_priority_max(SCHED_FIFO)};
    pthread_attr_setschedparam(&attr, &shparam);
    
    struct interruptAttr_gpio intAttr = {pin, callback, params, 0};
    ret = pthread_create(&tid, &attr, &interruptThreadArg_gpio, &intAttr);

    if(ret < 0) return ret;

    while(intAttr.copied == false) {};

    gpios[pin].interrupt = tid;
    return tid;
}


int attachInterrupt(int pin, int mode, void(*callback)())
{
    tryInitGpio();

    if(pin < 0) return -1;

    gpios[pin].line = gpiod_chip_get_line(rpi_chip, pin);
    gpiod_line_release(gpios[pin].line);

    detachInterrupt(pin);

    int ret;
    switch(mode)
    {
        case ISR_RISING:
            ret = gpiod_line_request_rising_edge_events(gpios[pin].line, RGPIO_CONSUMER);
            break;
        case ISR_FALLING:
            ret = gpiod_line_request_falling_edge_events(gpios[pin].line, RGPIO_CONSUMER);
            break;
        case ISR_BOTH:
            ret = gpiod_line_request_both_edges_events(gpios[pin].line, RGPIO_CONSUMER);
            break;
    }


    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    struct sched_param shparam = {sched_get_priority_max(SCHED_FIFO)};
    pthread_attr_setschedparam(&attr, &shparam);
    
    struct interruptAttr_gpio intAttr = {pin, callback, NULL, 0};
    ret = pthread_create(&tid, &attr, &interruptThread_gpio, &intAttr);

    if(ret < 0) return ret;

    while(intAttr.copied == false) {};
    
    gpios[pin].interrupt = tid;
    return tid;
}

void detachInterrupt(int pin)
{
    if (gpios[pin].interrupt != 0)
        pthread_cancel(gpios[pin].interrupt);
}