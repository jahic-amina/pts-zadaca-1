#include "radio.h"
#include "delay.h"
#include <string.h>

#define RX_FIFO_SIZE 512

static uint8_t rx_fifo[RX_FIFO_SIZE];
static uint16_t rx_fifo_head = 0;
static uint16_t rx_fifo_tail = 0;

static volatile uint8_t rx_buffer[PACKET_SIZE];

//provjeriti da li je bolje hardcodiraati kanal ili ga proslijediti kao argument funkciji initRADIO
void initRADIO(uint8_t channel, uint8_t * pkt)
{
    // Radio config
    NRF_RADIO->TXPOWER   = 0x00000004;									// 4 dBm Tx power
    NRF_RADIO->FREQUENCY = (uint32_t)channel;  										// 2400 MHz 
    NRF_RADIO->MODE = 0x00000000;										// 1Mbps BLE

	// Set BLE Inter Frame Space (T_IFS) interval to 150 us.
	// NRF_RADIO->TIFS = 150;

	
	// Enable data whitening, set the maximum payload length and set the
	// access address size (3 + 1 octets).
	//NRF_RADIO->PCNF1 = 0x02030000|(RADIO_MAX_PAYLOAD_LEN);

	// packet configuration:
	// bit 25 = 0 isljucuje se whiteen jer je custom protokol
	// bit 24 = 0 je little endian format
	// bit 18-16 = 3 znaci da je adresa 4B (3+1)
	// biti 15-8 = 0 znaci da se ne koristi S1 field (koji je 0B) i da se ne koristi statlen (koji je 0B)
	// biti 7-0 = 22 znaci da je maksimalna duzina payloada 22B
	NRF_RADIO->PCNF1 = 0x00030016;
	
		
	// Preset the address to use when receive and transmit packets (logical
	// address 0, which is assembled by base address BASE0 and prefix byte PREFIX0.AP0.
	NRF_RADIO->RXADDRESSES = 0x00000001;
	NRF_RADIO->TXADDRESS = 0x00000000;

	NRF_RADIO->CRCCNF = 0x00000000; //jer korsitimo custom protokol i proracunavamo CRC
	
	
	//koristimo custom protokol i proracunavamo CRC, tako da se ne koristi ni S1 field ni statlen field
	NRF_RADIO->PCNF0 = 0x00000000;
	
	
	NRF_RADIO->PACKETPTR = (uint32_t)pkt;
	
	//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
	// select channel, frequency and send the data
	//----------------------------------------------------------------
	// 4-bajtna lozinka ignorira sve pakete koji ne pocinju sa tom lozinkom, a 6-bajtna adresa ignorira sve pakete koji ne sadrze tu adresu
	NRF_RADIO->FREQUENCY = 2UL;
	NRF_RADIO->BASE0 = ((ADV_CHANNEL_ACCESS_ADDRESS) >> 8)&0xFFFFFF00;
	NRF_RADIO->PREFIX0 = ((ADV_CHANNEL_ACCESS_ADDRESS) >> 24)&0x000000FF;

	// init RF part
	NRF_RADIO->SHORTS |= 0x00000010;
	NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk; //trebat ce za sleep mode?
    NVIC_EnableIRQ(RADIO_IRQn);
}


void rxEnableRADIO(void)
{
	NRF_RADIO->PACKETPTR = (uint32_t)&rx_buffer[rx_fifo_head];
	NRF_RADIO->EVENTS_READY = 0;
	NRF_RADIO->EVENTS_END = 0;
	NRF_RADIO->TASKS_RXEN = 1;
	while (!NRF_RADIO->EVENTS_READY);
	NRF_RADIO->TASKS_START = 1;
}


void RADIO_IRQHandler(void)
{
	if(NRF_RADIO->EVENTS_END){
		NRF_RADIO->EVENTS_END = 0;
		uint16_t next_head = (rx_fifo_head + PACKET_SIZE) % RX_FIFO_SIZE;
		if (next_head != rx_fifo_tail) {
			memcpy(&rx_fifo[rx_fifo_head], (const void *)rx_buffer, PACKET_SIZE);
			rx_fifo_head = next_head;
		}

		rxEnableRADIO();
	}
}


void txPktRADIO(const fet_packet_t *pkt)
{
	//isljucujemo RX mod
	NRF_RADIO->EVENTS_DISABLED = 0x00000000;
	NRF_RADIO->TASKS_DISABLE = 0x00000001;
	while (NRF_RADIO->EVENTS_DISABLED == 0);

	NRF_RADIO->PACKETPTR = (uint32_t)pkt;
	NRF_RADIO->EVENTS_READY = 0;
	NRF_RADIO->TASKS_TXEN =1;

	while (NRF_RADIO->EVENTS_READY == 0);
	NRF_RADIO->TASKS_START = 1;

	while(NRF_RADIO->EVENTS_END == 0);
	NRF_RADIO->EVENTS_END = 0;

	rxEnableRADIO();

}


fet_packet_t *processRxFifoRADIO(void)
{
	while (rx_fifo_tail != rx_fifo_head)
	{
		uint16_t available = (rx_fifo_head - rx_fifo_tail + RX_FIFO_SIZE) % RX_FIFO_SIZE;
		if (available < PACKET_SIZE)
			break;

		fet_packet_t *pkt = (fet_packet_t *)&rx_fifo[rx_fifo_tail]; // privremeni buffer za parsiranje paketa
		rx_fifo_tail = (rx_fifo_tail + PACKET_SIZE) % RX_FIFO_SIZE;

		if (pkt->preamble != PREAMBLE_VALUE)      continue;
		if (!fet_packet_verify_checksum(pkt))   continue;

		return pkt; // vrati validan paket
	}

	return NULL;
}