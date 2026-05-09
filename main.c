#include <nrf52840.h>
#include <nrf52840_bitfields.h>
#include <stdint.h>
#include "rtc.h"
#include "uart.h"
#include "delay.h"
#include "radio.h"


//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// Power consumption test
// iBeacon using direct RF mode on 37, 38 & 39 channels
// Link Layer specification section 2.3, Core 4.1, page 2504
// Link Layer specification section 2.3.1.3, Core 4.1, page 2507
//
// adv_data PDU (39 octets):
// +--------+--------+---------+
// | Header |  AdvA  | AdvData |
// +--------+--------+---------+
//  2 octets 6 octets 31 octets
// --------------------------------------------------------------------- 
// Advertising chanels
// 37 - 2402 MHz
// 38 - 2426 MHz 
// 39 - 2480 MHz
//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww

// total BLE packet lenght can not be larger than 39B
// max packet payload is 31 bytes [in our case 30B]
volatile uint8_t adv_data[38] = {
	0x42, 0x24,															// [ 2B] - Header -> second byte (6 bits are length: 36B)
	0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,									// [ 5B] - Advertising Address 
	
	// Beacon prefix													   [ 9B]
	0x02, 0x01, 0x06,													// advertising flags
	0x1A, 0xFF, 														// advertising header
	0x00, 0x4C,															// Company ID
	0x02,																// iBeacon type
	0x15,		
	
	// Beacon UUID														  [16B]
	0xf7, 0x82, 0x6d, 0xa6, 
	0x4f, 0xa2, 0x4e, 0x98,	
	0x80, 0x24, 0xbc, 0x5b,	
	0x71, 0xe0, 0x89, 0x3e,

	// Major number														[ 2B] 
	0x17, 0xef,															// 6127 
	// Minor number														[ 2B]
	0x18, 0x54,															// 6228
	// Tx power	     													[ 1B]
	0x3C																// 60 dB
};

void initCLK(void);

int main(void)
{	
	initCLK();
	
	initUART0(6, 8, UART0_BAUDRATE_115200);// init UART 
	printUART0("\nwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n",0);
	printUART0("w nRF52 iBeacon test...\n",0);
	printUART0("-------------------------------------------------------\n",0);
	deinitUART0(6, 8);

	// init RTC1 with RC LF clock
	initWakeupRTC1(1000);
	
	
	uint32_t cnt = 0;
	while(1)
	{
		// tx iBeacon on all three advertising channels
		txDataRADIO(37, (uint8_t *)adv_data, 4);
		txDataRADIO(38, (uint8_t *)adv_data, 4);
		txDataRADIO(39, (uint8_t *)adv_data, 4);
		
				
		cnt++;
		initUART0(6, 8, UART0_BAUDRATE_115200);		
		printUART0("-> SYS: Tx [%d] completed\n",cnt);
		printUART0("-> SYS: sleeping...\n");	
		deinitUART0(6, 8);
		
		NRF_TIMER0->TASKS_STOP = 1;
		NRF_TIMER1->TASKS_STOP = 1;
		NRF_TIMER2->TASKS_STOP = 1;
		NRF_CLOCK->TASKS_HFCLKSTOP = 1;
		__SEV();
		__WFE();
		__WFE();
	}
	
	return 0;
}

void initCLK()
{
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
	{
		// Wait for the external oscillator to start up.
	}

	NRF_CLOCK->LFCLKSRC = (CLOCK_LFCLKSRC_EXTERNAL_Enabled << CLOCK_LFCLKSRC_EXTERNAL_Pos) ||
						  (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
		; // wait for the external oscillator to start up
}