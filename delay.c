#include "delay.h"

volatile uint32_t g_systim_time = 0;

void delay_ms(uint32_t ms)
{/// delay based on 16 bit TIMER2, max delay 2^32 ms
	uint32_t k;
	for(k=0;k<ms;k++)
	{
		delay_us(1000);
	}
}

void delay_us(uint16_t us)
{/// delay based on 16 bit TIMER2, max delay 2^16 us
	NRF_TIMER2->TASKS_CLEAR = 1;
	
	NRF_TIMER2->MODE = 0x0000;											// select counter mode
	NRF_TIMER2->PRESCALER = 0x0004; 									// 16 MHz clock, prescaler 16 -> 1us time step
	NRF_TIMER2->BITMODE = 0x0000;										// 16 bit timer mode
	
	NRF_TIMER2->CC[0] = us;
	NRF_TIMER2->SHORTS = 0x0001;
	NRF_TIMER2->TASKS_START = 1;
	

	while(NRF_TIMER2->EVENTS_COMPARE[0] == 0);
	NRF_TIMER2->EVENTS_COMPARE[0] = 0;
	NRF_TIMER2->TASKS_SHUTDOWN = 1;
}

void initSYSTIM(void)
{/// delay based on 16 bit TIMER1, max delay 2^16 us
	NRF_TIMER1->TASKS_SHUTDOWN = 1;
	NRF_TIMER1->TASKS_CLEAR = 1;
	
	NRF_TIMER1->MODE = 0x0000;											// select counter mode
	NRF_TIMER1->PRESCALER = 0x0004; 									// 16 MHz clock, prescaler 16 -> 1us time step
	NRF_TIMER1->BITMODE = 0x0000;										// 16 bit timer mode
	
	NRF_TIMER1->CC[0] = 1000;											// ticks are in 1ms
	//NRF_TIMER1->SHORTS = 0x0001;
	
	g_systim_time = 0;
	
	NRF_TIMER1->INTENSET = (1<<16);										// enable interrupt on TIMER1
	NVIC_EnableIRQ(TIMER1_IRQn);
		
	NRF_TIMER1->TASKS_START = 1;
}

uint32_t getSYSTIM(void)
{
	return g_systim_time;
}

void deinitSYSTIM(void)
{
	NVIC_DisableIRQ(TIMER1_IRQn);
	NRF_TIMER1->TASKS_SHUTDOWN = 1;
}

void TIMER1_IRQHandler(void)
{// timer 1 capture compare interrupt event execution 	
	NRF_TIMER1->EVENTS_COMPARE[0] = 0;									// clear the interrupt flag
	g_systim_time++;
		
	NRF_TIMER1->TASKS_CLEAR = 1;										// restart TIMER1
}

uint8_t chk4TimeoutSYSTIM(uint32_t t_beg, uint32_t t_period)
{	
	uint32_t time = g_systim_time;
	if(time >= t_beg)
	{
		if(time >= (t_beg + t_period))
			return (SYSTIM_TIMEOUT);
		else
			return (SYSTIM_KEEP_ALIVE);
	}
	else
	{
		uint32_t utmp32 = 0xFFFFFFFF - t_beg;
		if((time + utmp32) >= t_period)
			return (SYSTIM_TIMEOUT);
		else
			return (SYSTIM_KEEP_ALIVE);
	}
}

