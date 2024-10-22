#include "oled_display.h"

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "rtc.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "stm32f1xx_hal_rtc.h"

void set_current_time(struct date_info_t *date)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef DateToUpdate = {0};
    sTime.Hours = date->hour;
    sTime.Minutes = date->minute;
    sTime.Seconds = date->second;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK){
        Error_Handler();
    }
    DateToUpdate.WeekDay = date->week;
    DateToUpdate.Month = date->month;
    DateToUpdate.Year = (date->year-2000);
    DateToUpdate.Date = date->day;
    if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }
}

static uint8_t trans_month(uint8_t month)
{
    if(month  > 9) {
        return (month -6);
    }
    return month;
}

void display_smart_clock()
{
    uint8_t time_buf[64]={0};
    RTC_TimeTypeDef stTime={};
    RTC_DateTypeDef stDate={};
    memset(time_buf,0,sizeof(time_buf));
    ssd1306_SetCursor(2,0);
    ssd1306_WriteString("Smart Clock",Font_6x8,White);
    ssd1306_SetCursor(2,20);
    HAL_RTC_GetTime(&hrtc, &stTime,RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc,&stDate,RTC_FORMAT_BIN);
    sprintf(time_buf,"%02d-%02d-%02d %.2d:%.2d:%.2d",stDate.Year+2000,trans_month(stDate.Month),stDate.Date,
        stTime.Hours,stTime.Minutes,stTime.Seconds);
    ssd1306_WriteString(time_buf,Font_6x8,White);
    ssd1306_UpdateScreen();
}

void display_wait_check_time()
{
    ssd1306_SetCursor(2,0);
    ssd1306_WriteString("Smart Clock",Font_6x8,White);
    ssd1306_SetCursor(2,20);
    ssd1306_WriteString("Please wait ......",Font_6x8,White);
    ssd1306_UpdateScreen();
}

