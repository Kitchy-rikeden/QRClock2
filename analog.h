#ifndef ANALOG
#define ANALOG

#include "display.h"

void draw_hand(float r, float theta, Color_t color, Color_t matrix[32][32]);
void draw_background(Color_t color, Color_t matrix[32][32]);

#endif