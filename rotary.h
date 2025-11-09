#ifndef ROTARY
#define ROTARY

#include "hardware/gpio.h"

typedef struct
{
    uint pin_a, pin_b, pin_p;
    int hist_len;
    bool f_push;
    int f_rotate;

    int _state;
    uint64_t _a_hist, _b_hist, _p_hist;
    int _a_val, _b_val, _p_val;
} rotary_t;

void rotary_init(rotary_t *dev);
void rotary_main_loop(rotary_t *dev);

#endif