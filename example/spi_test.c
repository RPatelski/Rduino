// [Working]
// SPI MISO - GPIO9 
// SPI MOSI - GPIO10
// SPI CLK - GPIO11 
// SPI SS - GPIO8 

// compile
//	gcc spi_test.c -o spi_test -lrduino

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <r_spi.h>

uint8_t* tx;
uint8_t* rx;
int buf_len;

void gen_random(char* s, const int len)
{
  static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for(int i = 0; i < len; ++i)
    s[i] = alphanum[rand()%(sizeof(alphanum) - 1)];
  s[len] = 0;
}

void checkConsistency(int ret)
{
    if(ret <= 0)
    {
        printf("Transmission error\n");
        return;
    }

    for(int j = 0; j < buf_len; ++j)
    {
        printf("TX: %c, RX: %c\n", tx[j], rx[j]);

        if(tx[j] != rx[j])
        {
            printf("data inconsistency\n");
            return;
        }
    }
}

int main(int argc, char *argv[])
{
    buf_len = 32*1024;

	tx = (uint8_t*)malloc(buf_len);
    rx = (uint8_t*)malloc(buf_len);
   
    gen_random(tx, buf_len);     

    begin(0);

    setClockSpeed(0, 50e+6);

    transferInterrupt(0, tx, rx, buf_len, checkConsistency);

    while(1) 
    {
        
    };

	return 0;
}
