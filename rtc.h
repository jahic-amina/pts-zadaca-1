#ifndef __RTC_H_
#define __RTC_H_

#include <nrf52840.h>
#include <nrf52840_bitfields.h>
#include <system_nrf52840.h>
#include <stdint.h>
#include "uart.h"
#include "time.h"

#define RTC_TIMEOUT							0
#define RTC_KEEP_ALIVE						1


void 		initRTC1(void);
void 		initWakeupRTC1(uint32_t time);
void 		deinitRTC1(void);
uint32_t 	getRTC1(void);
uint8_t 	chk4TimeoutRTC1(uint32_t t_beg, uint32_t t_period);

uint32_t 	getTIMEDATE(void);
void 		setTIMEDATE(uint32_t timedate);
void		printTIMEDATE(void);
uint8_t 	chk4TimeoutTIMEDATE(uint32_t t_beg, uint32_t t_period);

#endif 
