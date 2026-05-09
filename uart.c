#include "uart.h"
#include "stdarg.h"

void initUART0(uint8_t tx, uint8_t rx, uint32_t baudrate)
{
	NRF_P0->PIN_CNF[tx] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
	NRF_P0->PIN_CNF[rx] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos);

	NRF_UART0->PSEL.TXD = (UART_PSEL_TXD_CONNECT_Connected << UART_PSEL_TXD_CONNECT_Pos) | (0 << UART_PSEL_TXD_PORT_Pos) | (tx << UART_PSEL_TXD_PIN_Pos);

	NRF_UART0->PSEL.RXD = (UART_PSEL_RXD_CONNECT_Connected << UART_PSEL_RXD_CONNECT_Pos) | (0 << UART_PSEL_RXD_PORT_Pos) | (rx << UART_PSEL_RXD_PIN_Pos);

	NRF_UART0->BAUDRATE = baudrate;
	NRF_UART0->ENABLE = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
	NRF_UART0->TASKS_STARTTX = 1;
	NRF_UART0->TASKS_STARTRX = 1;
	NRF_UART0->EVENTS_RXDRDY = 0;
}

void deinitUART0(uint8_t tx, uint8_t rx)
{
	NRF_UART0->PSEL.TXD = (UART_PSEL_TXD_CONNECT_Disconnected << UART_PSEL_TXD_CONNECT_Pos) | (0 << UART_PSEL_TXD_PORT_Pos) | (tx << UART_PSEL_TXD_PIN_Pos);

	NRF_UART0->PSEL.RXD = (UART_PSEL_RXD_CONNECT_Disconnected << UART_PSEL_RXD_CONNECT_Pos) | (0 << UART_PSEL_RXD_PORT_Pos) | (rx << UART_PSEL_RXD_PIN_Pos);

	NRF_UART0->ENABLE = (UART_ENABLE_ENABLE_Disabled << UART_ENABLE_ENABLE_Pos);
}

void putcharUART0(uint8_t data)
{
	NRF_UART0->TXD = data;
	while (NRF_UART0->EVENTS_TXDRDY != 1)
		;

	NRF_UART0->EVENTS_TXDRDY = 0;
}

void printUART0(char *str, ...)
{					  /// print text and up to 10 arguments!
	uint8_t rstr[40]; // 33 max -> 32 ASCII for 32 bits and NULL
	uint16_t k = 0;
	uint16_t arg_type;
	uint32_t utmp32;
	uint32_t *p_uint32;
	char *p_char;
	va_list vl;

	//va_start(vl, 10);													// always pass the last named parameter to va_start, for compatibility with older compilers
	va_start(vl, str); // always pass the last named parameter to va_start, for compatibility with older compilers
	while (str[k] != 0x00)
	{
		if (str[k] == '%')
		{
			if (str[k + 1] != 0x00)
			{
				switch (str[k + 1])
				{
				case ('b'):
				{ // binary
					if (str[k + 2] == 'b')
					{ // byte
						utmp32 = va_arg(vl, int);
						arg_type = (PRINT_ARG_TYPE_BINARY_BYTE);
					}
					else if (str[k + 2] == 'h')
					{ // half word
						utmp32 = va_arg(vl, int);
						arg_type = (PRINT_ARG_TYPE_BINARY_HALFWORD);
					}
					else if (str[k + 2] == 'w')
					{ // word
						utmp32 = va_arg(vl, uint32_t);
						arg_type = (PRINT_ARG_TYPE_BINARY_WORD);
					}
					else
					{ // default word
						utmp32 = va_arg(vl, uint32_t);
						arg_type = (PRINT_ARG_TYPE_BINARY_WORD);
						k--;
					}

					k++;
					p_uint32 = &utmp32;
					break;
				}
				case ('d'):
				{ // decimal
					if (str[k + 2] == 'b')
					{ // byte
						utmp32 = va_arg(vl, int);
						arg_type = (PRINT_ARG_TYPE_DECIMAL_BYTE);
					}
					else if (str[k + 2] == 'h')
					{ // half word
						utmp32 = va_arg(vl, int);
						arg_type = (PRINT_ARG_TYPE_DECIMAL_HALFWORD);
					}
					else if (str[k + 2] == 'w')
					{ // word
						utmp32 = va_arg(vl, uint32_t);
						arg_type = (PRINT_ARG_TYPE_DECIMAL_WORD);
					}
					else
					{ // default word
						utmp32 = va_arg(vl, uint32_t);
						arg_type = (PRINT_ARG_TYPE_DECIMAL_WORD);
						k--;
					}

					k++;
					p_uint32 = &utmp32;
					break;
				}
				case ('c'):
				{ // character
					char tchar = va_arg(vl, int);
					putcharUART0(tchar);
					arg_type = (PRINT_ARG_TYPE_CHARACTER);
					break;
				}
				case ('s'):
				{ // string
					p_char = va_arg(vl, char *);
					sprintUART0((uint8_t *)p_char);
					arg_type = (PRINT_ARG_TYPE_STRING);
					break;
				}
				case ('f'):
				{											// float
					uint64_t utmp64 = va_arg(vl, uint64_t); // convert double to float representation IEEE 754
					uint32_t tmp1 = utmp64 & 0x00000000FFFFFFFF;
					tmp1 = tmp1 >> 29;
					utmp32 = utmp64 >> 32;
					utmp32 &= 0x07FFFFFF;
					utmp32 = utmp32 << 3;
					utmp32 |= tmp1;
					if (utmp64 & 0x8000000000000000)
						utmp32 |= 0x80000000;

					if (utmp64 & 0x4000000000000000)
						utmp32 |= 0x40000000;

					p_uint32 = &utmp32;

					arg_type = (PRINT_ARG_TYPE_FLOAT);
					//arg_type = (PRINT_ARG_TYPE_HEXADECIMAL_WORD);
					//arg_type = (PRINT_ARG_TYPE_BINARY_WORD);
					break;
				}
				case ('x'):
				{ // hexadecimal
					if (str[k + 2] == 'b')
					{ // byte
						utmp32 = (uint32_t)va_arg(vl, int);
						arg_type = (PRINT_ARG_TYPE_HEXADECIMAL_BYTE);
					}
					else if (str[k + 2] == 'h')
					{ // half word
						utmp32 = (uint32_t)va_arg(vl, int);
						arg_type = (PRINT_ARG_TYPE_HEXADECIMAL_HALFWORD);
					}
					else if (str[k + 2] == 'w')
					{ // word
						utmp32 = va_arg(vl, uint32_t);
						arg_type = (PRINT_ARG_TYPE_HEXADECIMAL_WORD);
					}
					else
					{
						utmp32 = va_arg(vl, uint32_t);
						arg_type = (PRINT_ARG_TYPE_HEXADECIMAL_WORD);
						k--;
					}

					k++;
					p_uint32 = &utmp32;
					break;
				}
				default:
				{
					utmp32 = 0;
					p_uint32 = &utmp32;
					arg_type = (PRINT_ARG_TYPE_UNKNOWN);
					break;
				}
				}

				if (arg_type & (PRINT_ARG_TYPE_MASK_CHAR_STRING))
				{
					getStr4NumMISC(arg_type, p_uint32, rstr);
					sprintUART0(rstr);
				}
				k++;
			}
		}
		else
		{ // not a '%' char -> print the char
			putcharUART0(str[k]);
			if (str[k] == '\n')
				putcharUART0('\r');
		}
		k++;
	}

	va_end(vl);
	return;
}

void sprintUART0(unsigned char *str)
{
	uint16_t k = 0;

	while (str[k] != '\0')
	{
		putcharUART0(str[k]);
		if (str[k] == '\n')
			putcharUART0('\r');
		k++;

		if (k == MAX_PRINT_STRING_SIZE)
			break;
	}
}
