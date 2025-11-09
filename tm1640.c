#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdint.h>
#include "tm1640.h"

#define TM1640_CMD1 0x40   // data command
#define TM1640_CMD2 0xC0   // address command
#define TM1640_CMD3 0x80   // display control command
#define TM1640_DSP_ON 0x08 // 0x08 display on
#define TM1640_DELAY_US 10 // 10us delay between clk/dio pulses

void tm1640_delay()
{
    sleep_us(TM1640_DELAY_US);
}

void tm1640_start(const tm1640_t *dev)
{
    for (int i = 0; i < TM1640_CHANNELS; i++)
        gpio_put(dev->pin_dios[i], 0);
    tm1640_delay();
    gpio_put(dev->pin_clk, 0);
    tm1640_delay();
}

void tm1640_stop(const tm1640_t *dev)
{
    for (int i = 0; i < TM1640_CHANNELS; i++)
        gpio_put(dev->pin_dios[i], 0);
    tm1640_delay();
    gpio_put(dev->pin_clk, 1);
    tm1640_delay();
    for (int i = 0; i < TM1640_CHANNELS; i++)
        gpio_put(dev->pin_dios[i], 1);
    tm1640_delay();
}

void tm1640_write_byte(const tm1640_t *dev, uint8_t data)
{
    for (int i = 0; i < 8; i++)
    {
        int bit = (data >> i) & 1;
        for (int ch = 0; ch < TM1640_CHANNELS; ch++)
        {
            gpio_put(dev->pin_dios[ch], bit);
        }
        tm1640_delay();
        gpio_put(dev->pin_clk, 1);
        tm1640_delay();
        gpio_put(dev->pin_clk, 0);
        tm1640_delay();
    }
}

void tm1640_write_data_cmd(const tm1640_t *dev)
{
    tm1640_start(dev);
    tm1640_write_byte(dev, TM1640_CMD1);
    tm1640_stop(dev);
}

void tm1640_write_dsp_ctrl(const tm1640_t *dev)
{
    tm1640_start(dev);
    tm1640_write_byte(dev, TM1640_CMD3 | TM1640_DSP_ON | dev->brightness);
    tm1640_stop(dev);
}

// brightness: 0 - 7
void tm1640_init(const tm1640_t *dev, const uint64_t data[16][2])
{
    gpio_init(dev->pin_clk);
    gpio_set_dir(dev->pin_clk, GPIO_OUT);
    gpio_put(dev->pin_clk, 1);

    for (int i = 0; i < TM1640_CHANNELS; i++)
    {
        gpio_init(dev->pin_dios[i]);
        gpio_set_dir(dev->pin_dios[i], GPIO_OUT);
        gpio_put(dev->pin_dios[i], 1);
    }

    tm1640_write_ints(dev, data);
    tm1640_write_dsp_ctrl(dev);
}

// data: 16ch x 2color. Each color 64bit = 8row x 8col. Little endian.
void tm1640_write_ints(const tm1640_t *dev, const uint64_t data[16][2])
{
    tm1640_write_data_cmd(dev);
    tm1640_start(dev);
    tm1640_write_byte(dev, TM1640_CMD2); // Start address

    for (int color = 0; color < 2; color++)
    {
        for (int row = 0; row < 8; row++)
        {
            for (int col = 0; col < 8; col++)
            {
                for (int ch = 0; ch < TM1640_CHANNELS; ch++)
                {
                    int bit = (data[ch][color] >> (row * 8 + col)) & 1;
                    gpio_put(dev->pin_dios[ch], bit);
                }
                tm1640_delay();
                gpio_put(dev->pin_clk, 1);
                tm1640_delay();
                gpio_put(dev->pin_clk, 0);
                tm1640_delay();
            }
        }
    }

    tm1640_stop(dev);
}
