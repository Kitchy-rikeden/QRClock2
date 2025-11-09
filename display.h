#ifndef DISPLAY
#define DISPLAY
#include <stdint.h>
#include "tm1640.h"

typedef enum
{
    OFF = 0b00,
    RED = 0b01,
    GREEN = 0b10,
    ORANGE = 0b11,
} Color_t;

void display_init();
void display_convert_matrix_to_array(const Color_t matrix[32][32], uint64_t array[TM1640_CHANNELS][2]);
void display_print_string_to_matrix(char *s, int line, int col, Color_t color, Color_t matrix[32][32]);
void display_clear_matrix(Color_t matrix[32][32]);

#endif