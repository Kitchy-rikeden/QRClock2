#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "ds1302.h"

#define DS1302_REG_SECOND 0x80
#define DS1302_REG_MINUTE 0x82
#define DS1302_REG_HOUR 0x84
#define DS1302_REG_DATE 0x86
#define DS1302_REG_MONTH 0x88
#define DS1302_REG_DAY 0x8A
#define DS1302_REG_YEAR 0x8C
#define DS1302_REG_WP 0x8E
#define DS1302_REG_CTRL 0x90
#define DS1302_REG_RAM 0xC0
#define DS1302_DELAY_US 1

void ds1302_delay()
{
    sleep_us(DS1302_DELAY_US);
}

int bcd_to_int(int data)
{
    return (data / 16) * 10 + (data % 16);
}

int int_to_bcd(int data)
{
    return (data / 10) * 16 + (data % 10);
}

void write_byte(ds1302_t *dev, int data)
{
    gpio_set_dir(dev->pin_dio, GPIO_OUT);
    for (int i = 0; i < 8; i++)
    {
        gpio_put(dev->pin_clk, 0);
        ds1302_delay();
        gpio_put(dev->pin_dio, (data >> i) & 1);
        ds1302_delay();
        gpio_put(dev->pin_clk, 1);
        ds1302_delay();
    }
    gpio_set_dir(dev->pin_dio, GPIO_IN);
    gpio_put(dev->pin_clk, 0);
    ds1302_delay();
}

int read_byte(ds1302_t *dev)
{
    int res = 0;
    for (int i = 0; i < 8; i++)
    {
        res |= (gpio_get(dev->pin_dio) << i);
        ds1302_delay();
        gpio_put(dev->pin_clk, 1);
        ds1302_delay();
        gpio_put(dev->pin_clk, 0);
        ds1302_delay();
    }
    return res;
}

void set_reg(ds1302_t *dev, int reg, int data)
{
    gpio_put(dev->pin_ce, 1);
    ds1302_delay();
    write_byte(dev, reg);
    write_byte(dev, data);
    gpio_put(dev->pin_ce, 0);
    ds1302_delay();
}

int get_reg(ds1302_t *dev, int reg)
{
    gpio_put(dev->pin_ce, 1);
    ds1302_delay();
    write_byte(dev, reg | 1);
    int res = read_byte(dev);
    gpio_put(dev->pin_ce, 0);
    ds1302_delay();
    return res;
}

void ds1302_set_datetime(ds1302_t *dev, datetime_t datetime)
{
    set_reg(dev, DS1302_REG_WP, 0);                                 // Disable WriteProtect
    set_reg(dev, DS1302_REG_SECOND, int_to_bcd(datetime.sec));      // 同時に ClockHalt = 0 も設定
    set_reg(dev, DS1302_REG_MINUTE, int_to_bcd(datetime.min));      //
    set_reg(dev, DS1302_REG_HOUR, int_to_bcd(datetime.hour));       // 24 hour モード
    set_reg(dev, DS1302_REG_DATE, int_to_bcd(datetime.day));        //
    set_reg(dev, DS1302_REG_MONTH, int_to_bcd(datetime.month));     //
    set_reg(dev, DS1302_REG_DAY, int_to_bcd(datetime.dotw));        //
    set_reg(dev, DS1302_REG_YEAR, int_to_bcd(datetime.year % 100)); //
    set_reg(dev, DS1302_REG_CTRL, 0xA5);                            // Trickle Charge 1 Diode, 2k ohm
}

datetime_t ds1302_get_datetime(ds1302_t *dev)
{
    datetime_t res;
    res.sec = bcd_to_int(get_reg(dev, DS1302_REG_SECOND));
    res.min = bcd_to_int(get_reg(dev, DS1302_REG_MINUTE));
    res.hour = bcd_to_int(get_reg(dev, DS1302_REG_HOUR));
    res.day = bcd_to_int(get_reg(dev, DS1302_REG_DATE));
    res.month = bcd_to_int(get_reg(dev, DS1302_REG_MONTH));
    res.dotw = bcd_to_int(get_reg(dev, DS1302_REG_DAY));
    res.year = 2000 + bcd_to_int(get_reg(dev, DS1302_REG_YEAR));

    return res;
}

void ds1302_init(ds1302_t *dev)
{
    gpio_init(dev->pin_clk);
    gpio_set_dir(dev->pin_clk, GPIO_OUT);
    gpio_put(dev->pin_clk, 0);

    gpio_init(dev->pin_dio);
    gpio_set_dir(dev->pin_dio, GPIO_IN);

    gpio_init(dev->pin_ce);
    gpio_set_dir(dev->pin_ce, GPIO_OUT);
    gpio_put(dev->pin_ce, 0);
}
