#include "esp8266.h"

#include <stdio.h>
#include <string.h>

#include "stm32f1xx_hal.h"
#include "usart.h"
#include "oled_display.h"

#define ESP8266_AT_RSP_LEN  64

typedef enum
{
    ESP8266_STATE_INIT,
    ESP8266_STATE_WIFI,
    ESP8266_STATE_NTP,
    ESP8266_STATE_GET_TIME,
    ESP8266_STATE_FINISH,
}ENU_ESP8266_STATE;

struct esp8266_at_rsp
{
    uint8_t is_check_rsp;
    uint8_t is_success;
    char rsp[ESP8266_AT_RSP_LEN];
};

static struct esp8266_at_rsp s_esp8266_at_rsp;

static uint8_t s_esp8266_state;

int str2int(char *str)
{
    int res = 0;
    for (int i = 0; str[i]; i++) {
        res = res * 10 + (str[i] - '0');
    }
    return res;
}

uint8_t get_date_week(const char *str)
{
    if(0 == strcmp(str, "Sun")) {
        return RTC_WEEKDAY_SUNDAY;
    }else if(0 == strcmp(str, "Mon")) {
        return RTC_WEEKDAY_MONDAY;
    }else if(0 == strcmp(str, "Tue")) {
        return RTC_WEEKDAY_TUESDAY;
    }else if(0 == strcmp(str, "Wed")) {
        return RTC_WEEKDAY_WEDNESDAY;
    }else if(0 == strcmp(str, "Thu")) {
        return RTC_WEEKDAY_THURSDAY;
    }else if(0 == strcmp(str, "Fri")) {
        return RTC_WEEKDAY_FRIDAY;
    }else {
        return RTC_WEEKDAY_SATURDAY;
    }
}

uint8_t get_date_month(const char *str)
{
    if(0 == strcmp(str, "Jan")) {
        return RTC_MONTH_JANUARY;
    }else if(0 == strcmp(str, "Feb")) {
        return RTC_MONTH_FEBRUARY;
    }else if(0 == strcmp(str, "Mar")) {
        return RTC_MONTH_MARCH;
    }else if(0 == strcmp(str, "Apr")) {
        return RTC_MONTH_APRIL;
    }else if(0 == strcmp(str, "May")) {
        return RTC_MONTH_MAY;
    }else if(0 == strcmp(str, "Jun")) {
        return RTC_MONTH_JUNE;
    }else if(0 == strcmp(str, "Jul")) {
        return RTC_MONTH_JULY;
    }else if(0 == strcmp(str, "Aug")) {
        return RTC_MONTH_AUGUST;
    }else if(0 == strcmp(str, "Sep")) {
        return RTC_MONTH_SEPTEMBER;
    }else if(0 == strcmp(str, "Oct")) {
        return RTC_MONTH_OCTOBER;
    }else if(0 == strcmp(str, "Nov")) {
        return RTC_MONTH_NOVEMBER;
    }else {
        return RTC_MONTH_DECEMBER;
    }
}


static void phrase_date_str(char *str, struct date_info_t *date)
{
    char dayOfWeek[4] = "";
    char month[4] = "";
    char dayStr[4] = "";
    char hourStr[4] = "";
    char minuteStr[4] = "";
    char secondStr[4] = "";
    char yearStr[8] = "";

    const char *p = str + strlen("+CIPSNTPTIME:");
    // 提取星期几
    strncpy(dayOfWeek, p, 3);
    p += 4; // 跳过 "Tue" 和空格
    // 提取月份
    strncpy(month, p, 3);
    p += 4; // 跳过 "Oct" 和空格
    // 提取日期
    strncpy(dayStr, p, 2);
    p += 3; // 跳过 "22" 和空格
    // 提取小时
    strncpy(hourStr, p, 2);
    p += 3; // 跳过 "09" 和冒号 ":"
    // 提取分钟
    strncpy(minuteStr, p, 2);
    p += 3; // 跳过 "37" 和冒号 ":"
    // 提取秒数
    strncpy(secondStr, p, 2);
    p += 3; // 跳过 "25" 和空格
    // 提取年份
    strncpy(yearStr, p, 4);
    date->year = str2int(yearStr);
    date->day = str2int(dayStr);
    date->hour = str2int(hourStr);
    date->minute = str2int(minuteStr);
    date->second = str2int(secondStr);
    date->week = get_date_week(dayOfWeek);
    date->month = get_date_month(month);
    char debug[32];
    memset(debug, 0, 32);
    sprintf(debug,"%d->%d->%d:%d->%d->%d\r\n",date->year,date->month,date->day,date->hour,date->minute,date->second);
    uart_debug_msg(debug);
}

void check_and_phrase_current_time(char *msg)
{
    struct date_info_t date;
    phrase_date_str(msg, &date);
    if(date.year != 1970) {
        set_current_time(&date);
        s_esp8266_state = ESP8266_STATE_FINISH;
        uart_debug_msg("Set Time success\r\n");
    }
}


void uart1_rcv_callback(uint8_t *rcv_buf, uint8_t len)
{
    if(s_esp8266_at_rsp.is_check_rsp) {;
        if(strstr((char*)rcv_buf,s_esp8266_at_rsp.rsp)) {
            s_esp8266_at_rsp.is_success = 1;
            if(0 == strcmp(s_esp8266_at_rsp.rsp,"CIPSNTPTIME")) {
                check_and_phrase_current_time((char*)rcv_buf);
            }
        }
    }
}

static void uart1_send_cmd(uint8_t *cmd)
{
    while(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) != SET);
    HAL_UART_Transmit(&huart1, cmd, strlen((char*)cmd),1000);
}

int8_t esp8266_at(uint8_t timeout, char *cmd, char*rsp)
{
    memset(&s_esp8266_at_rsp, 0, sizeof(s_esp8266_at_rsp));
    clear_uart1_rcv_buf();
    if((rsp != NULL) && (strlen(rsp) > 0)) {
        s_esp8266_at_rsp.is_check_rsp = 1;
        strncpy(s_esp8266_at_rsp.rsp,rsp,ESP8266_AT_RSP_LEN-1);
    }
    uart1_send_cmd((uint8_t*)cmd);
    HAL_Delay(3000);
    if(!s_esp8266_at_rsp.is_check_rsp) {
        return  0;
    }
    while ((s_esp8266_at_rsp.is_check_rsp) && (timeout --)) {
        if(s_esp8266_at_rsp.is_success) {
            return 0;
        }
        HAL_Delay(10);
    }
    if(s_esp8266_at_rsp.is_success) {
        return 0;
    }
    return 1;
}

static void esp8266_init(void)
{
    esp8266_at(100,"AT+RST\r\n",NULL);
    esp8266_at(100,"ATE0\r\n",NULL);
    s_esp8266_state = ESP8266_STATE_WIFI;
}

static void esp8266_set_wifi()
{
    if(0 != esp8266_at(100,"AT+CWMODE=1\r\n",NULL)) {
        s_esp8266_state = ESP8266_STATE_INIT;
        return;
    }
    if(0 != esp8266_at(100,"AT+CWJAP=\"HUAWEI_B311_AA33\",\"kdla2024++\"\r\n","WIFI GOT IP")) {
        s_esp8266_state = ESP8266_STATE_INIT;
        return;
    }
    s_esp8266_state = ESP8266_STATE_NTP;
}

static void esp8266_set_ntp()
{
    if(0 != esp8266_at(100,"AT+CIPSNTPCFG=1,8,\"cn.ntp.org.cn\",\"ntp.sjtu.edu.cn\"\r\n","OK")) {
        s_esp8266_state = ESP8266_STATE_INIT;
        return;
    }
    s_esp8266_state = ESP8266_STATE_GET_TIME;
}

static void esp8266_get_time()
{
    if(0 != esp8266_at(200,"AT+CIPSNTPTIME?\r\n","CIPSNTPTIME")) {
        s_esp8266_state = ESP8266_STATE_INIT;
    }
}

void start_esp8266_process(void)
{
    char buf[32] ={0};
    if(ESP8266_STATE_FINISH == s_esp8266_state) {
        return;
    }
    sniprintf(buf,sizeof(buf),"Current state:%d\r\n",s_esp8266_state);
    uart_debug_msg(buf);
    switch (s_esp8266_state) {
        case ESP8266_STATE_INIT:
            esp8266_init();
        break;
        case ESP8266_STATE_WIFI:
            esp8266_set_wifi();
        break;
        case ESP8266_STATE_NTP:
            esp8266_set_ntp();
        break;
        case  ESP8266_STATE_GET_TIME:
            esp8266_get_time();
        break;
        default:
            uart_debug_msg("State is error");
            s_esp8266_state = ESP8266_STATE_INIT;
        break;
    }
}

uint8_t is_play_clock()
{
    if(s_esp8266_state == ESP8266_STATE_FINISH) {
        return 1;
    }
    return 0;
}

