#include <pthread.h>
#include <gpiod.h>

#include "r_gpio.h"

#define RGPIO_CONSUMER "r_gpio"

//handle to RPi GPIO chip, opened only once at initialization
struct gpiod_chip* rpi_chip = NULL;

void tryInitGpio()
{
    if(rpi_chip == NULL)
        rpi_chip = gpiod_chip_open_by_name("gpiochip0");
}

int setMode(int pin, int mode)
{
    tryInitGpio();

    struct gpiod_line* line = gpiod_chip_get_line(rpi_chip, pin);
    gpiod_line_release(line);

    switch(mode)
    {
        case OUTPUT:
            return gpiod_line_request_output(line, RGPIO_CONSUMER, 0);

        case INPUT:
            return gpiod_line_request_input(line, RGPIO_CONSUMER);

        case ISR_RISING:
            return gpiod_line_request_rising_edge_events(line, RGPIO_CONSUMER);

        case ISR_FALLING:
            return gpiod_line_request_falling_edge_events(line, RGPIO_CONSUMER);

        case ISR_BOTH:
            return gpiod_line_request_both_edges_events(line, RGPIO_CONSUMER);
            
        default:
            return -1;
    }
}

void digitalWrite(int pin, int value)
{
    struct gpiod_line* line = gpiod_chip_get_line(rpi_chip, pin);
    gpiod_line_set_value(line, value);
    return;
}

int digitalRead(int pin)
{
    struct gpiod_line* line = gpiod_chip_get_line(rpi_chip, pin);
    return gpiod_line_get_value(line);
}

int waitInterrupt(int pin)
{
    struct gpiod_line* line = gpiod_chip_get_line(rpi_chip, pin);

    int ret;
    ret = gpiod_line_event_wait(line, NULL);
    if(ret < 0) return ret;

    struct gpiod_line_event event;
    ret =  gpiod_line_event_read(line, &event);

    return event.event_type;
}

struct interruptAttr_gpio
{
    int pin;
    void(*callback)(int);
    volatile bool copied;
};

void* interruptThread_gpio(void* attr)
{
    int pin = ((struct interruptAttr_gpio*)attr)->pin;
    void(*callback)(int) = ((struct interruptAttr_gpio*)attr)->callback;

    ((struct interruptAttr_gpio*)attr)->copied = true;

    while(1)
    {
        int edge = waitInterrupt(pin);
        if(edge > 0)
            callback(edge);
    }

    return NULL;
}

int attachInterrupt(int pin, int mode, void(*callback)(int))
{
    tryInitGpio();
    
    struct gpiod_line* line = gpiod_chip_get_line(rpi_chip, pin);
    gpiod_line_release(line);

    int ret;
    switch(mode)
    {
        case ISR_RISING:
            ret = gpiod_line_request_rising_edge_events(line, RGPIO_CONSUMER);
            break;
        case ISR_FALLING:
            ret = gpiod_line_request_falling_edge_events(line, RGPIO_CONSUMER);
            break;
        case ISR_BOTH:
            ret = gpiod_line_request_both_edges_events(line, RGPIO_CONSUMER);
            break;
    }


    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    struct sched_param shparam = {sched_get_priority_max(SCHED_FIFO)};
    pthread_attr_setschedparam(&attr, &shparam);
    
    struct interruptAttr_gpio intAttr = {pin, callback, 0};
    ret = pthread_create(&tid, &attr, &interruptThread_gpio, &intAttr);

    if(ret < 0) return ret;

    while(intAttr.copied == false) {};
    return tid;
}