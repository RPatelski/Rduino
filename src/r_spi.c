#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/fcntl.h> 

#include "r_spi.h"

#define SPI_CNT 6

int spis[SPI_CNT] = {0};
int spis_mode[SPI_CNT] = {0};
int spis_speed[SPI_CNT] = {0};

void begin(int spi)
{
    if (spi >= SPI_CNT || spis[spi] != 0) 
        return;

    char device[16] = {0};
    sprintf(device, "/dev/spidev%d.0", spi);
    spis[spi] = open(device, O_RDWR);

    int bits = 8;

    ioctl(spis[spi], SPI_IOC_WR_MAX_SPEED_HZ, &spis_speed[spi]);
    ioctl(spis[spi], SPI_IOC_WR_MODE, &spis_mode[spi]);
	ioctl(spis[spi], SPI_IOC_WR_BITS_PER_WORD, &bits);
}

void end(int spi)
{
    if (spi >= SPI_CNT || spis[spi] == 0) 
        return;

    close(spis[spi]);
    spis[spi] = 0;
}

void setBitOrder(int spi, int order)
{
    if (spi >= SPI_CNT || spis[spi] == 0) 
        return;

    if(order == LSBFIRST)
        spis_mode[spi] |= SPI_LSB_FIRST;
    else
        spis_mode[spi] &= ~SPI_LSB_FIRST;

    ioctl(spis[spi], SPI_IOC_WR_MODE, &spis_mode[spi]);
}

void setClockSpeed(int spi, int speed)
{
    if (spi >= SPI_CNT || spis[spi] == 0) 
        return;

    spis_speed[spi] = speed;

    ioctl(spis[spi], SPI_IOC_WR_MAX_SPEED_HZ, &spis_speed[spi]);
}

void setDataMode(int spi, int mode)
{
    if (spi >= SPI_CNT || spis[spi] == 0) 
        return;

    spis_mode[spi] &= ~0x03;

    switch(mode)
    {
        case SPI_MODE0:
            spis_mode[spi] |= SPI_MODE_0;
            break;

        case SPI_MODE1:
            spis_mode[spi] |= SPI_MODE_1;
            break;

        case SPI_MODE2:
            spis_mode[spi] |= SPI_MODE_2;
            break;

        case SPI_MODE3:
            spis_mode[spi] |= SPI_MODE_3;
            break;
    }

    ioctl(spis[spi], SPI_IOC_WR_MODE, &spis_mode[spi]);
}

void setNoCS(int spi, int disable)
{
    if (spi >= SPI_CNT || spis[spi] == 0) 
        return;

    if(disable == DISABLE_CS)
        spis_mode[spi] |= SPI_NO_CS;
    else
        spis_mode[spi] &= ~SPI_NO_CS;

    ioctl(spis[spi], SPI_IOC_WR_MODE, &spis_mode[spi]);
}

int transfer(int spi, void* tx, void* rx, int length)
{
    struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = length,
		.delay_usecs = 0,
		.speed_hz = spis_speed[spi],
		.bits_per_word = 8,
	};

    return ioctl(spis[spi], SPI_IOC_MESSAGE(1), &tr);
}

struct interruptAttr_spi
{
    int spi;
    struct spi_ioc_transfer tr;
    void(*callback)(int);
    volatile bool copied;
};

void* interruptThread_spi(void* attr)
{
    int spi = ((struct interruptAttr_spi*)attr)->spi;
    struct spi_ioc_transfer tr = ((struct interruptAttr_spi*)attr)->tr;
    void(*callback)(int) = ((struct interruptAttr_spi*)attr)->callback;

    ((struct interruptAttr_spi*)attr)->copied = true;

    int ret =  ioctl(spis[spi], SPI_IOC_MESSAGE(1), &tr);
    
    callback(ret);

    return NULL;
}

int transferInterrupt(int spi, void* tx, void* rx, int length, void(*callback)(int))
{
    struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = length,
		.delay_usecs = 0,
		.speed_hz = spis_speed[spi],
		.bits_per_word = 8,
	};

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    struct sched_param shparam = {sched_get_priority_max(SCHED_FIFO)};
    pthread_attr_setschedparam(&attr, &shparam);
    
    struct interruptAttr_spi intAttr = {spi, tr, callback, 0};
    int ret = pthread_create(&tid, &attr, &interruptThread_spi, &intAttr);
    
    if(ret < 0) return ret;

    while(intAttr.copied == false) {};
    return tid;
}
