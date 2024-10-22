#ifndef ESP8266_H
#define ESP8266_H
#include <stdint.h>

void uart1_rcv_callback(uint8_t *rcv_buf, uint8_t len);
void start_esp8266_process(void);
uint8_t is_play_clock();
#endif //ESP8266_H
