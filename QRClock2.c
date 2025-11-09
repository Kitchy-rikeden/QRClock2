#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "tm1640.h"
#include "display.h"
#include "rotary.h"
#include "ds1302.h"
#include "ntp_client.h"
#include "qrencode.h"
#include "analog.h"

typedef enum
{
    QR = 0,
    ANA = 1,
    DIG = 2,
    MENU = 3,
} mode_t;

const char *weekday_japanese[7] = {"日", "月", "火", "水", "木", "金", "土"};

// ピン
const tm1640_t tm1640 = {
    .pin_clk = 16,
    .pin_dios = {3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12},
    .brightness = 7,
};
rotary_t rotary = {
    .pin_a = 20,
    .pin_b = 21,
    .pin_p = 22,
    .hist_len = 64,
    .f_push = false,
    .f_rotate = 0,
};
ds1302_t ds1302 = {
    .pin_clk = 17,
    .pin_dio = 18,
    .pin_ce = 19,
};

uint64_t display_array[TM1640_CHANNELS][2];
Color_t display_matrix[32][32];
bool f_update = false; // 画面更新フラグ
struct repeating_timer timer;

bool timer_callback(struct repeating_timer *t)
{
    f_update = true;
    return true;
}

void init()
{
    stdio_init_all();
    for (int i = 0; i < TM1640_CHANNELS; i++)
        display_array[i][0] = display_array[i][1] = 0;
    tm1640_init(&tm1640, display_array);
    display_init();
    rotary_init(&rotary);
    ds1302_init(&ds1302);
    ntp_init();
    add_repeating_timer_ms(-1000, timer_callback, NULL, &timer);
}

void show_menu(int cursor, char ntp_status)
{
    display_clear_matrix(display_matrix);

    display_print_string_to_matrix((cursor == 0 ? ">" : " "), 0, 0, RED, display_matrix);
    display_print_string_to_matrix("QR", 0, 1, RED, display_matrix);
    display_print_string_to_matrix((cursor == 1 ? ">" : " "), 1, 0, RED, display_matrix);
    display_print_string_to_matrix("ANA", 1, 1, RED, display_matrix);
    display_print_string_to_matrix((cursor == 2 ? ">" : " "), 2, 0, RED, display_matrix);
    display_print_string_to_matrix("DIG", 2, 1, RED, display_matrix);
    display_print_string_to_matrix((cursor == 3 ? ">" : " "), 3, 0, RED, display_matrix);
    display_print_string_to_matrix("NTP", 3, 1, RED, display_matrix);
    char s[2];
    s[0] = ntp_status;
    s[1] = '\0';
    display_print_string_to_matrix(s, 3, 4, RED, display_matrix);

    display_convert_matrix_to_array(display_matrix, display_array);
    tm1640_write_ints(&tm1640, display_array);
}

void show_digital(datetime_t *dt)
{
    display_clear_matrix(display_matrix);
    char s[21];
    snprintf(s, 21, "%d/%2d/%2d%2d:%02d  :%02d", dt->year, dt->month, dt->day, dt->hour, dt->min, dt->sec);
    display_print_string_to_matrix(s, 0, 0, GREEN, display_matrix);
    display_convert_matrix_to_array(display_matrix, display_array);
    tm1640_write_ints(&tm1640, display_array);
}

void show_qr(datetime_t *dt)
{
    char buf[50];
    // UTF-8で保存すること.
    sprintf(buf, "%d年%d月%d日 (%s) %d時%d分%d秒", dt->year, dt->month, dt->day, weekday_japanese[(dt->dotw) % 7], dt->hour, dt->min, dt->sec);
    QRcode *qrcode = QRcode_encodeString(buf, 3, QR_ECLEVEL_M, QR_MODE_8, 1);

    if (!qrcode)
    {
        printf("QR encode failed\n");
        return;
    }

    display_clear_matrix(display_matrix);
    for (int y = 0; y < qrcode->width; y++)
    {
        for (int x = 0; x < qrcode->width; x++)
        {
            int idx = y * qrcode->width + x;
            if (qrcode->data[idx] & 1)
            {
                display_matrix[y + 1][x + 1] = ORANGE;
            }
        }
    }

    display_convert_matrix_to_array(display_matrix, display_array);
    tm1640_write_ints(&tm1640, display_array);
    QRcode_free(qrcode);
}

void show_analog(datetime_t *dt)
{
    draw_background(RED, display_matrix);
    // 秒針
    float t1 = M_PI * 2 * dt->sec / 60 - M_PI_2;
    draw_hand(12, t1, GREEN, display_matrix);
    // 長針
    float t2 = M_PI * 2 * dt->min / 60 - M_PI_2;
    draw_hand(10, t2, ORANGE, display_matrix);
    // 短針
    float t3 = M_PI * 2 * (dt->hour % 12 * 5 + dt->min / 12) / 60 - M_PI_2;
    draw_hand(8, t3, ORANGE, display_matrix);

    display_convert_matrix_to_array(display_matrix, display_array);
    tm1640_write_ints(&tm1640, display_array);
}

int main()
{
    sleep_ms(500);
    init();

    int cursor = 0;
    char ntp_status = ' ';
    mode_t mode = QR;
    datetime_t dt;

    while (true)
    {
        rotary_main_loop(&rotary);

        // イベント処理
        if (rotary.f_push)
        {
            rotary.f_push = false;
            if (mode == MENU)
            {
                if (cursor == 3)
                {
                    ntp_status = '_';
                    show_menu(cursor, ntp_status);
                    if (ntp_get_time(&dt, 10 * 1000))
                    {
                        ntp_status = 'o';
                        show_menu(cursor, ntp_status);
                        ds1302_set_datetime(&ds1302, dt);
                    }
                    else
                    {
                        ntp_status = 'x';
                        show_menu(cursor, ntp_status);
                    }
                }
                else
                {
                    mode = cursor;
                    f_update = true;
                }
            }
            else
            {
                mode = MENU;
                ntp_status = ' ';
                show_menu(cursor, ntp_status);
            }
        }
        if (rotary.f_rotate != 0)
        {
            if (mode == MENU)
            {
                cursor = (cursor + rotary.f_rotate + 4) % 4;
                show_menu(cursor, ntp_status);
            }
            rotary.f_rotate = 0;
        }
        if (f_update && mode != MENU)
        {
            f_update = false;
            dt = ds1302_get_datetime(&ds1302);

            if (mode == QR)
            {
                show_qr(&dt);
            }
            else if (mode == DIG)
            {
                show_digital(&dt);
            }
            else if (mode == ANA)
            {
                show_analog(&dt);
            }
        }
    }
}
