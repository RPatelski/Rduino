#ifndef _R_SPI_H_
#define _R_SPI_H_

enum bit_order 
{
    LSBFIRST,
    MSBFIRST
};

enum data_mode
{
    SPI_MODE0,
    SPI_MODE1,
    SPI_MODE2,
    SPI_MODE3,
};

enum no_cs
{
    ENABLE_CS,
    DISABLE_CS
};

void begin(int spi);
void end(int spi);

void setBitOrder(int spi, int order);
void setClockSpeed(int spi, int speed);
void setDataMode(int spi, int mode);
void setNoCS(int spi, int disable);

int transfer(int spi, void* tx, void* rx, int length);
int transferInterrupt(int spi, void* tx, void* rx, int length, void(*callback)(int));

#endif