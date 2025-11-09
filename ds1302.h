#ifndef DS1302
#define DS1302
#include "hardware/gpio.h"

typedef struct
{
    uint pin_clk, pin_dio, pin_ce;
} ds1302_t;

void ds1302_set_datetime(ds1302_t *dev, datetime_t datetime);
datetime_t ds1302_get_datetime(ds1302_t *dev);
void ds1302_init(ds1302_t *dev);

#endif