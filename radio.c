#include "radio.h"


void txDataRADIO(uint8_t channel, uint8_t * frame, int8_t power)
{
	//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
	// init HF clock
	//----------------------------------------------------------------
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;									// start 16 MHz crystal oscillator 
	NRF_CLOCK->TASKS_HFCLKSTART = 0x00000001;
	while(NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);							// wait for the external oscillator to start up

	//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
	// init RADIO
	//----------------------------------------------------------------
	// set BLE 1Mbps mode
    NRF_RADIO->MODE = (RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos);

	// Set BLE Inter Frame Space (T_IFS) interval to 150 us.
	NRF_RADIO->TIFS = 150;

	// Enable data whitening, set the maximum payload length and set the
	// access address size (3 + 1 octets).
	NRF_RADIO->PCNF1 = 0x02030000|(RADIO_MAX_PAYLOAD_LEN);

	// Preset the address to use when receive and transmit packets (logical
	// address 0, which is assembled by base address BASE0 and prefix byte PREFIX0.AP0.
	NRF_RADIO->RXADDRESSES = 0x00000001;
	NRF_RADIO->TXADDRESS = 0x00000000;

	// Set CRC length to 3 bytes and do not include access address in calculation of CRC
	NRF_RADIO->CRCCNF = 0x00000103;
	// set CRC polinomial
	NRF_RADIO->CRCPOLY = 0x100065B;

	// Configure the header size. The nRF52840 has 3 fields before the
	// payload field: S0 (1B), LENGTH(up to 256B) and S1 (0B), they define PDU header.
	NRF_RADIO->PCNF0 = 0x00000108;
	
	// set tx power and input tx buffer
	NRF_RADIO->TXPOWER = power;										
	NRF_RADIO->PACKETPTR = (uint32_t)frame;
	
	//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
	// select channel, frequency and send the data
	//----------------------------------------------------------------
	NRF_RADIO->DATAWHITEIV = channel & 0x3F;							// set channel
	NRF_RADIO->FREQUENCY = getFreq4ChRADIO(channel);					// set channel frequency
	NRF_RADIO->BASE0 = ((ADV_CHANNEL_ACCESS_ADDRESS) << 8)&0xFFFFFF00;
	
	NRF_RADIO->PREFIX0 = ((ADV_CHANNEL_ACCESS_ADDRESS) >> 24)&0x000000FF;
	NRF_RADIO->CRCINIT = 0x00555555;
	
	NRF_RADIO->TASKS_TXEN = 0x00000001;
	
	// enable radio and wait for confirmation
	NRF_RADIO->EVENTS_READY = 0U;
    NRF_RADIO->TASKS_TXEN   = 1;
    while (NRF_RADIO->EVENTS_READY == 0U);

	// initiate transmission and wait for completion
    NRF_RADIO->EVENTS_END  = 0U;
    NRF_RADIO->TASKS_START = 1U;
    while (NRF_RADIO->EVENTS_END == 0U);

    // disable radio and wait for confirmation							// you can ommit this and use deinit radio
    NRF_RADIO->EVENTS_DISABLED = 0U; 
    NRF_RADIO->TASKS_DISABLE = 1U;
    while (NRF_RADIO->EVENTS_DISABLED == 0U);
    
    
    ////wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
	//// disable RADIO
	////------------------------------------------------------------------
    //NRF_RADIO->SHORTS = 0;
    //NRF_RADIO->EVENTS_DISABLED = 0;
    //NRF_RADIO->TASKS_DISABLE = 1;
    //while (NRF_RADIO->EVENTS_DISABLED == 0);

    //NRF_RADIO->EVENTS_DISABLED = 0;
    //NRF_RADIO->TASKS_RXEN = 0;
    //NRF_RADIO->TASKS_TXEN = 0;
    
}

int8_t getFreq4ChRADIO(uint8_t ch)
{/// conver channel to actual frequency in MHz which will be added to 2400 MHz
	
	switch(ch) 
	{
		case(37):
		{
			return 2;
		}
		case 38:
		{
			return 26;
		}
		case 39:
		{
			return 80;
		}
		default:
		{
			if (ch > 39)
				return -1;
			else if (ch < 11)
				return 4 + (2*ch);
			else
				return 6 + (2*ch);
		}
	}
}


