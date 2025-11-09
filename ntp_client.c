/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

typedef struct NTP_T_
{
    ip_addr_t ntp_server_address;
    struct udp_pcb *ntp_pcb;
    int dns_result, ntp_result; // 0: waiting, 1: success, -1: fail
    uint32_t unix_time;
} NTP_T;

#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970

// Make an NTP request
static void ntp_request(NTP_T *state)
{
    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

// Call back with a DNS result
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
    NTP_T *state = (NTP_T *)arg;
    if (ipaddr)
    {
        state->ntp_server_address = *ipaddr;
        printf("ntp address %s\n", ipaddr_ntoa(ipaddr));
        state->dns_result = 1;
    }
    else
    {
        state->dns_result = -1;
        printf("ntp dns request failed\n");
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    NTP_T *state = (NTP_T *)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0)
    {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t ntp_time = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        state->unix_time = ntp_time - NTP_DELTA;
        state->ntp_result = 1;
    }
    else
    {
        state->ntp_result = -1;
        printf("invalid ntp response\n");
    }
    pbuf_free(p);
}

void ntp_init()
{
    if (cyw43_arch_init())
    {
        printf("failed to cyw43_arch_init\n");
    }
    cyw43_arch_enable_sta_mode();
}

// Wifiに接続し, NTPサーバーから時刻を取得する. 成功した場合に true を返す. 結果は引数の result に格納される.
bool ntp_get_time(datetime_t *result, uint32_t timeout_ms)
{
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, timeout_ms))
    {
        printf("failed to connect Wifi\n");
        return false;
    }

    NTP_T state;
    state.dns_result = 0;
    state.ntp_result = 0;
    state.ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state.ntp_pcb)
    {
        printf("failed to create pcb\n");
        return false;
    }

    cyw43_arch_lwip_begin();
    int err = dns_gethostbyname(NTP_SERVER, &state.ntp_server_address, ntp_dns_found, &state);
    cyw43_arch_lwip_end();

    if (err == ERR_INPROGRESS)
    {
        // DNS 待機
        while (state.dns_result == 0 && absolute_time_diff_us(get_absolute_time(), deadline) > 0)
        {
            cyw43_arch_poll();
            sleep_ms(1);
        }
    }
    else
    {
        printf("invalid DNS response.\n");
        goto FAIL;
    }

    if (state.dns_result != 1)
    {
        printf("failed to get response from DNS.\n");
        goto FAIL;
    }

    udp_recv(state.ntp_pcb, ntp_recv, &state);
    ntp_request(&state);

    // NTP 待機
    while (state.ntp_result == 0 && absolute_time_diff_us(get_absolute_time(), deadline) > 0)
    {
        cyw43_arch_poll();
        sleep_ms(1); // これがないと動かない!
    }

    if (state.ntp_result != 1)
    {
        printf("failed to get response from NTP.\n");
        goto FAIL;
    }

    time_t jst_time = state.unix_time + 9 * 60 * 60;
    struct tm *tm = gmtime(&jst_time);

    result->year = tm->tm_year + 1900;
    result->month = tm->tm_mon + 1;
    result->day = tm->tm_mday;
    result->hour = tm->tm_hour;
    result->min = tm->tm_min;
    result->sec = tm->tm_sec;
    result->dotw = tm->tm_wday; // 0 = Sunday

    udp_remove(state.ntp_pcb);
    return true;

FAIL:
    udp_remove(state.ntp_pcb);
    return false;
}
