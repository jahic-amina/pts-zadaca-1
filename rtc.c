#include "rtc.h"

volatile uint32_t g_rtc1_time = 0;
volatile uint16_t g_rtc1_subsec = 0;
volatile uint32_t g_rtc1_sec = 0;


void initRTC1(void)
{/// tick with frequency of 1.007 Hz
	NRF_RTC1->TASKS_STOP = 1;
	NVIC_DisableIRQ(RTC1_IRQn);
	NRF_RTC1->EVENTS_COMPARE[0] = 0;
	NRF_RTC1->TASKS_CLEAR = 1;
	NRF_RTC1->EVTENCLR = 0xFFFFFFFF;
	NRF_RTC1->INTENCLR = 0xFFFFFFFF;
		
		
	g_rtc1_time = 0;
    g_rtc1_subsec = 0;
	while(NRF_RTC1->PRESCALER != 0)
		NRF_RTC1->PRESCALER = 0;
												
	NRF_RTC1->CC[0] = 33;
	
	NRF_RTC1->EVTENSET = 0x00010000;
	NRF_RTC1->INTENSET = 0x00010000;
	NRF_RTC1->EVENTS_COMPARE[0] = 0;
	NVIC_EnableIRQ(RTC1_IRQn);
	NRF_RTC1->TASKS_START = 1;
}

void initWakeupRTC1(uint32_t time)
{
	NVIC_DisableIRQ(RTC1_IRQn);
	NRF_RTC1->EVENTS_COMPARE[0] = 0;
	NRF_RTC1->TASKS_STOP = 1;
	NRF_RTC1->TASKS_CLEAR = 1;
	NRF_RTC1->EVTENCLR = 0xFFFFFFFF;
	NRF_RTC1->INTENCLR = 0xFFFFFFFF;
	
	while(NRF_RTC1->PRESCALER != 32)
		NRF_RTC1->PRESCALER = 32;
										
	NRF_RTC1->CC[0] = time;
	
	NRF_RTC1->EVTENSET = 0x00010000;
	NRF_RTC1->INTENSET = 0x00010000;
	NVIC_EnableIRQ(RTC1_IRQn);
	NRF_RTC1->TASKS_START = 1;
}

void deinitRTC1(void)
{
	NVIC_DisableIRQ(RTC1_IRQn);
	NRF_RTC1->TASKS_STOP = 1;
	NRF_RTC1->EVTENCLR = 0xFFFFFFFF;
	NRF_RTC1->INTENCLR = 0xFFFFFFFF;
	NRF_RTC1->EVENTS_COMPARE[0] = 0;
}

void RTC1_IRQHandler(void)
{
    if (NRF_RTC1->EVENTS_COMPARE[0] != 0)
    {
        NRF_RTC1->EVENTS_COMPARE[0] = 0;   // clear interrupt flag
        NRF_RTC1->TASKS_CLEAR = 1;         // restart RTC1

        g_rtc1_time++;
        g_rtc1_sec++;   // initWakeupRTC1(1000) fires ~once per second
    }
}

uint32_t getRTC1(void)
{
	return g_rtc1_time;
}

uint8_t chk4TimeoutRTC1(uint32_t t_beg, uint32_t t_period)
{	
	uint32_t rtc_time = g_rtc1_time;
	if(rtc_time >= t_beg)
	{
		if(rtc_time >= (t_beg + t_period))
			return (RTC_TIMEOUT);
		else
			return (RTC_KEEP_ALIVE);
	}
	else
	{
		uint32_t utmp32 = 0xFFFFFFFF - t_beg;
		if((rtc_time + utmp32) >= t_period)
			return (RTC_TIMEOUT);
		else
			return (RTC_KEEP_ALIVE);
	}
}

uint32_t getTIMEDATE(void)
{
	return g_rtc1_sec;
}

void setTIMEDATE(uint32_t timedate)
{
	g_rtc1_sec = timedate;
}

void printTIMEDATE(void)
{
	struct tm * ttm;
	ttm = localtime((time_t *)&g_rtc1_sec);
	ttm->tm_year += 1900;
	
	printUART0("\n-> TIMEDATE: %d:%d:%d - %d.%d.%d\n",ttm->tm_hour, ttm->tm_min, ttm->tm_sec, ttm->tm_mday, ttm->tm_mon + 1, ttm->tm_year);
}

uint8_t chk4TimeoutTIMEDATE(uint32_t t_beg, uint32_t t_period)
{	
	uint32_t rtc_time = g_rtc1_sec;
	if(rtc_time >= t_beg)
	{
		if(rtc_time >= (t_beg + t_period))
			return (RTC_TIMEOUT);
		else
			return (RTC_KEEP_ALIVE);
	}
	else
	{
		uint32_t utmp32 = 0xFFFFFFFF - t_beg;
		if((rtc_time + utmp32) >= t_period)
			return (RTC_TIMEOUT);
		else
			return (RTC_KEEP_ALIVE);
	}
}

