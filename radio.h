#ifndef __IBEACON_H_
#define __IBEACON_H_

#include <stdint.h>
#include <nrf52840.h>
#include <nrf52840_bitfields.h>
#include "uart.h"

// Link Layer specification Section 2.1.2, Core 4.1 page 2503 
#define ADV_CHANNEL_ACCESS_ADDRESS			0x8E89BED6

#define RADIO_MAX_PAYLOAD_LEN					37
#define RADIO_MAX_PDU							39

void txDataRADIO(uint8_t channel, uint8_t * frame, int8_t power);
int8_t getFreq4ChRADIO(uint8_t ch);

#endif 
