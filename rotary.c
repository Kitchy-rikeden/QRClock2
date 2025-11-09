#include <stdint.h>
#include "hardware/gpio.h"
#include "rotary.h"

// エンコーダの回転方向
#define DIR_CW 0x10  // 時計回り
#define DIR_CCW 0x20 // 反時計回り

// 状態定数
#define R_START 0x0
#define R_CW_1 0x1
#define R_CW_2 0x2
#define R_CW_3 0x3
#define R_CCW_1 0x4
#define R_CCW_2 0x5
#define R_CCW_3 0x6
#define R_ILLEGAL 0x7

#define STATE_MASK 0x07
#define DIR_MASK 0x30

// ハーフステップ用の遷移テーブル
const uint8_t transition_table_half_step[8][4] = {
    {R_CW_3, R_CW_2, R_CW_1, R_START},            // R_START
    {R_CW_3 | DIR_CCW, R_START, R_CW_1, R_START}, // R_CW_1
    {R_CW_3 | DIR_CW, R_CW_2, R_START, R_START},  // R_CW_2
    {R_CW_3, R_CCW_2, R_CCW_1, R_START},          // R_CW_3
    {R_CW_3, R_CW_2, R_CCW_1, R_START | DIR_CW},  // R_CCW_1
    {R_CW_3, R_CCW_2, R_CW_3, R_START | DIR_CCW}, // R_CCW_2
    {R_START, R_START, R_START, R_START},         // R_CCW_3
    {R_START, R_START, R_START, R_START},         // R_ILLEGAL
};

void rotary_init(rotary_t *dev)
{
    gpio_init(dev->pin_a);
    gpio_set_dir(dev->pin_a, GPIO_IN);
    gpio_pull_up(dev->pin_a);

    gpio_init(dev->pin_b);
    gpio_set_dir(dev->pin_b, GPIO_IN);
    gpio_pull_up(dev->pin_b);

    gpio_init(dev->pin_p);
    gpio_set_dir(dev->pin_p, GPIO_IN);
    gpio_pull_up(dev->pin_p);

    dev->_state = R_START;
    dev->_a_hist = 0xAAAAAAAAAAAAAAAA;
    dev->_b_hist = 0xAAAAAAAAAAAAAAAA;
    dev->_p_hist = 0xAAAAAAAAAAAAAAAA;
    dev->_a_val = 1;
    dev->_b_val = 1;
    dev->_p_val = 1;
}

// 回転量を符号付きの整数で返す. CCW: 1, CW: -1
int process_rotary_pins(rotary_t *dev)
{
    int clk_dt_pins = (dev->_a_val << 1) | dev->_b_val;

    dev->_state = transition_table_half_step[dev->_state & STATE_MASK][clk_dt_pins];
    int direction = dev->_state & DIR_MASK;

    int incr = 0;
    if (direction == DIR_CW)
        incr = 1;
    else if (direction == DIR_CCW)
        incr = -1;

    return incr;
}

// 1: Rising edge, -1: Falling edge, 0: Otherwise
int debounce_detect_edge(uint pin, uint64_t *hist, int *val, uint64_t mask)
{
    *hist = (*hist << 1) | gpio_get(pin);
    int new_val = -1;
    if ((*hist & mask) == mask)
        new_val = 1;
    else if ((*hist & mask) == 0)
        new_val = 0;

    if (new_val != -1)
    {
        int old_val = *val;
        *val = new_val;
        if (old_val == 0 && new_val == 1)
            return 1;
        else if (old_val == 1 && new_val == 0)
            return -1;
        else
            return 0;
    }
    else
    {
        return 0;
    }
}

void rotary_main_loop(rotary_t *dev)
{
    uint64_t mask = dev->hist_len >= 64 ? UINT64_MAX : (1ull << dev->hist_len) - 1;

    int edge_a = debounce_detect_edge(dev->pin_a, &dev->_a_hist, &dev->_a_val, mask);
    int edge_b = debounce_detect_edge(dev->pin_b, &dev->_b_hist, &dev->_b_val, mask);
    int edge_p = debounce_detect_edge(dev->pin_p, &dev->_p_hist, &dev->_p_val, mask);

    if (edge_a != 0 || edge_b != 0)
    {
        int incr = process_rotary_pins(dev);
        if (incr != 0)
            dev->f_rotate = incr;
    }
    if (edge_p == -1)
    {
        dev->f_push = true;
    }
}