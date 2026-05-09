#ifndef __UART0_H_
#define __UART0_H_

#include <stdbool.h>
#include <stdint.h>
#include <nrf52840.h>
#include <nrf52840_bitfields.h>
#include <system_nrf52840.h>
#include "misc.h"

#define UART0_BAUDRATE_9600					0x00275000	
#define UART0_BAUDRATE_115200				0x01D7E000	
#define UART0_BAUDRATE_230400				0x03AFB000	
#define UART0_BAUDRATE_460800				0x075F7000	
#define UART0_BAUDRATE_921600				0x0EBED000
			
void initUART0(uint8_t tx, uint8_t rx, uint32_t baudrate);
void deinitUART0(uint8_t tx, uint8_t rx);
void putcharUART0(uint8_t data);
void printUART0(char * str, ... );
void sprintUART0(unsigned char * str);

#endif 
