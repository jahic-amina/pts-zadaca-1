#ifndef __DELAY_H_
#define __DELAY_H_

#include <stdbool.h>
#include <stdint.h>
#include <nrf52840.h>
#include <nrf52840_bitfields.h>
#include <system_nrf52840.h>

#define SYSTIM_TIMEOUT							0
#define SYSTIM_KEEP_ALIVE						1

void 		delay_ms(uint32_t ms);
void 		delay_us(uint16_t us);

void 		initSYSTIM(void);
void 		deinitSYSTIM(void);
uint32_t 	getSYSTIM(void);
uint8_t 	chk4TimeoutSYSTIM(uint32_t t_beg, uint32_t t_period);
#endif 
