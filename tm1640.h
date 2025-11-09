#ifndef TM1640
#define TM1640
#include "hardware/gpio.h"

#define TM1640_CHANNELS 16

typedef struct
{
    uint pin_clk;
    uint pin_dios[TM1640_CHANNELS];
    uint brightness;
} tm1640_t;

void tm1640_init(const tm1640_t *dev, const uint64_t data[16][2]);
void tm1640_write_ints(const tm1640_t *dev, const uint64_t data[16][2]);

#endif