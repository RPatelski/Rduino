#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/fcntl.h> 

#include "r_spi.h"

#define SPI_CNT 6

struct spi_info
{
    int spi;
    int mode;
    int speed;
};

struct spi_info spis[SPI_CNT];


void begin(int spi)
{
    if (spi >= SPI_CNT || spis[spi].spi != 0) 
        return;

    char device[16] = {0};
    sprintf(device, "/dev/spidev%d.0", spi);
    spis[spi].spi = open(device, O_RDWR);

    int bits = 8;

    ioctl(spis[spi].spi, SPI_IOC_WR_MAX_SPEED_HZ, &spis[spi].speed);
    ioctl(spis[spi].spi, SPI_IOC_WR_MODE, &spis[spi].mode);
	ioctl(spis[spi].spi, SPI_IOC_WR_BITS_PER_WORD, &bits);
}

void end(int spi)
{
    if (spi >= SPI_CNT || spis[spi].spi == 0) 
        return;

    close(spis[spi].spi);
    spis[spi].spi = 0;
}

void setBitOrder(int spi, int order)
{
    if (spi >= SPI_CNT || spis[spi].spi == 0) 
        return;

    if(order == LSBFIRST)
        spis[spi].mode |= SPI_LSB_FIRST;
    else
        spis[spi].mode &= ~SPI_LSB_FIRST;

    ioctl(spis[spi].spi, SPI_IOC_WR_MODE, &spis[spi].mode);
}

void setClockSpeed(int spi, int speed)
{
    if (spi >= SPI_CNT || spis[spi].spi == 0) 
        return;

    spis[spi].speed = speed;

    ioctl(spis[spi].spi, SPI_IOC_WR_MAX_SPEED_HZ, &spis[spi].speed);
}

void setDataMode(int spi, int mode)
{
    if (spi >= SPI_CNT || spis[spi].spi == 0) 
        return;

    spis[spi].mode &= ~0x03;

    switch(mode)
    {
        case SPI_MODE0:
            spis[spi].mode |= SPI_MODE_0;
            break;

        case SPI_MODE1:
            spis[spi].mode |= SPI_MODE_1;
            break;

        case SPI_MODE2:
            spis[spi].mode |= SPI_MODE_2;
            break;

        case SPI_MODE3:
            spis[spi].mode |= SPI_MODE_3;
            break;
    }

    ioctl(spis[spi].spi, SPI_IOC_WR_MODE, &spis[spi].mode);
}

void setNoCS(int spi, int disable)
{
    if (spi >= SPI_CNT || spis[spi].spi == 0) 
        return;

    if(disable == DISABLE_CS)
        spis[spi].mode |= SPI_NO_CS;
    else
        spis[spi].mode &= ~SPI_NO_CS;

    ioctl(spis[spi].spi, SPI_IOC_WR_MODE, &spis[spi].mode);
}

int transfer(int spi, void* tx, void* rx, int length)
{
    struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = length,
		.delay_usecs = 0,
		.speed_hz = spis[spi].speed,
		.bits_per_word = 8,
	};

    return ioctl(spis[spi].spi, SPI_IOC_MESSAGE(1), &tr);
}

struct interruptAttr_spi
{
    int spi;
    struct spi_ioc_transfer tr;
    void(*callback)();
    void* params;
    volatile bool copied;
};

void* interruptThread_spi(void* attr)
{
    int spi = ((struct interruptAttr_spi*)attr)->spi;
    struct spi_ioc_transfer tr = ((struct interruptAttr_spi*)attr)->tr;
    void(*callback)() = ((struct interruptAttr_spi*)attr)->callback;

    ((struct interruptAttr_spi*)attr)->copied = true;

    int ret =  ioctl(spis[spi].spi, SPI_IOC_MESSAGE(1), &tr);
    
    if(ret > 0)
        callback();

    return NULL;
}

void* interruptThreadArg_spi(void* attr)
{
    int spi = ((struct interruptAttr_spi*)attr)->spi;
    struct spi_ioc_transfer tr = ((struct interruptAttr_spi*)attr)->tr;
    void(*callback)(void*) = ((struct interruptAttr_spi*)attr)->callback;
    void* params = ((struct interruptAttr_spi*)attr)->params;
    ((struct interruptAttr_spi*)attr)->copied = true;

    int ret =  ioctl(spis[spi].spi, SPI_IOC_MESSAGE(1), &tr);
    if(ret > 0)
        callback(params);

    return NULL;
}

int transferInterruptArg(int spi, void* tx, void* rx, int length, void(*callback)(void*), void* params)
{
    struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = length,
		.delay_usecs = 0,
		.speed_hz = spis[spi].speed,
		.bits_per_word = 8,
	};

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    struct sched_param shparam = {sched_get_priority_max(SCHED_FIFO)};
    pthread_attr_setschedparam(&attr, &shparam);
    
    struct interruptAttr_spi intAttr = {spi, tr, callback, params, 0};
    int ret = pthread_create(&tid, &attr, &interruptThreadArg_spi, &intAttr);
    
    if(ret < 0) return ret;

    while(intAttr.copied == false) {};
    return tid;
}


int transferInterrupt(int spi, void* tx, void* rx, int length, void(*callback)(int))
{
    struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = length,
		.delay_usecs = 0,
		.speed_hz = spis[spi].speed,
		.bits_per_word = 8,
	};

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    struct sched_param shparam = {sched_get_priority_max(SCHED_FIFO)};
    pthread_attr_setschedparam(&attr, &shparam);
    
    struct interruptAttr_spi intAttr = {spi, tr, callback, NULL, 0};
    int ret = pthread_create(&tid, &attr, &interruptThread_spi, &intAttr);
    
    if(ret < 0) return ret;

    while(intAttr.copied == false) {};
    return tid;
}
