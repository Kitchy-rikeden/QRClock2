#ifndef NTP_CLIENT
#define NTP_CLIENT

void ntp_init();
bool ntp_get_time(datetime_t *result, uint32_t timeout_ms);

#endif
