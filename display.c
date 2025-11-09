#include <stdint.h>
#include "display.h"
#include "tm1640.h"

typedef struct
{
    char c;
    uint8_t data[6];
} font_t;

typedef struct
{
    int ch;
    uint64_t bit;
} pos_t;

const font_t font[] = {
    {'0', {0b0111110, 0b1000101, 0b1001001, 0b1010001, 0b0111110, 0}},
    {'1', {0b0000000, 0b0100001, 0b1111111, 0b0000001, 0b0000000, 0}},
    {'2', {0b0100001, 0b1000011, 0b1000101, 0b1001001, 0b0110001, 0}},
    {'3', {0b1000010, 0b1000001, 0b1010001, 0b1101001, 0b1000110, 0}},
    {'4', {0b0001100, 0b0010100, 0b0100100, 0b1111111, 0b0000100, 0}},
    {'5', {0b1110010, 0b1010001, 0b1010001, 0b1010001, 0b1001110, 0}},
    {'6', {0b0011110, 0b0101001, 0b1001001, 0b1001001, 0b0000110, 0}},
    {'7', {0b1000000, 0b1000111, 0b1001000, 0b1010000, 0b1100000, 0}},
    {'8', {0b0110110, 0b1001001, 0b1001001, 0b1001001, 0b0110110, 0}},
    {'9', {0b0110000, 0b1001001, 0b1001001, 0b1001010, 0b0111100, 0}},
    {'A', {0b0111111, 0b1000100, 0b1000100, 0b1000100, 0b0111111, 0}},
    {'D', {0b1111111, 0b1000001, 0b1000001, 0b0100010, 0b0011100, 0}},
    {'G', {0b0111110, 0b1000001, 0b1001001, 0b1001001, 0b1101111, 0}},
    {'I', {0b0000000, 0b1000001, 0b1111111, 0b1000001, 0b0000000, 0}},
    {'N', {0b1111111, 0b0010000, 0b0001000, 0b0000100, 0b1111111, 0}},
    {'P', {0b1111111, 0b1001000, 0b1001000, 0b1001000, 0b0110000, 0}},
    {'Q', {0b0111110, 0b1000001, 0b1000101, 0b1000010, 0b0111101, 0}},
    {'R', {0b1111111, 0b1001000, 0b1001100, 0b1001010, 0b0110001, 0}},
    {'T', {0b1000000, 0b1000000, 0b1111111, 0b1000000, 0b1000000, 0}},
    {'o', {0b0011100, 0b0100010, 0b0100010, 0b0100010, 0b0011100, 0}},
    {'x', {0b0100010, 0b0010100, 0b0001000, 0b0010100, 0b0100010, 0}},
    {'>', {0b1000001, 0b0100010, 0b0010100, 0b0001000, 0b0000000, 0}},
    {'/', {0b0000010, 0b0000100, 0b0001000, 0b0010000, 0b0100000, 0}},
    {'_', {0b0000001, 0b0000001, 0b0000001, 0b0000001, 0b0000001, 0}},
    {':', {0b0000000, 0b0000000, 0b0010100, 0b0000000, 0b0000000, 0}},
    {'\0', {0, 0, 0, 0, 0, 0}},
};

// 32 x 32 のマトリクスから 16 ch x 64 bit のデータへの変換テーブル
pos_t pos_table[32][32];

const uint8_t *get_font(char c)
{
    for (int i = 0;; i++)
    {
        if (font[i].c == c || font[i].c == '\0')
            return font[i].data;
    }
}

void display_init()
{
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            int ch = (i / 8) + (3 - (j / 8)) * 4;
            int row = i % 8;
            int col = 7 - (j % 8);
            pos_table[i][j] = (pos_t){ch, 1ull << (row * 8 + col)};
        }
    }
}

void display_convert_matrix_to_array(const Color_t matrix[32][32], uint64_t array[TM1640_CHANNELS][2])
{
    for (int i = 0; i < TM1640_CHANNELS; i++)
    {
        array[i][0] = 0;
        array[i][1] = 0;
    }

    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            int ch = pos_table[i][j].ch;
            uint64_t bit = pos_table[i][j].bit;

            if (matrix[i][j] & RED)
                array[ch][0] |= bit;
            if (matrix[i][j] & GREEN)
                array[ch][1] |= bit;
        }
    }
}

// line: 0-3, col: 0-4. 指定した位置から配置し自動で改行
void display_print_string_to_matrix(char *s, int line, int col, Color_t color, Color_t matrix[32][32])
{
    for (int i = 0; s[i] != '\0'; i++)
    {
        const uint8_t *f = get_font(s[i]);
        for (int j = 0; j < 6; j++)
        {
            for (int k = 0; k < 8; k++)
            {
                if ((f[j] >> (7 - k)) & 1)
                    matrix[line * 8 + k][col * 6 + j + 1] = color;
                else
                    matrix[line * 8 + k][col * 6 + j + 1] = OFF;
            }
        }

        col++;
        if (col == 5)
        {
            col = 0;
            line++;
        }
    }
}

void display_clear_matrix(Color_t matrix[32][32])
{
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            matrix[i][j] = OFF;
        }
    }
}
