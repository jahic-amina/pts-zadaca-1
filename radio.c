#include "radio.h"
#include "delay.h"
#include <string.h>

#define RX_FIFO_SIZE 512

static uint8_t rx_fifo[RX_FIFO_SIZE];
static volatile uint16_t rx_fifo_head = 0;
static volatile uint16_t rx_fifo_tail = 0;

/* Used as a discard target when the FIFO is full */
static uint8_t rx_discard[PACKET_SIZE];

/* Set by rxEnableRADIO: 1 if DMA targets the FIFO, 0 if it targets rx_discard */
static volatile uint8_t rx_dma_to_fifo = 0;

void initRADIO(void)
{
    NRF_RADIO->TXPOWER   = 0x00000004;      // 4 dBm Tx power
    NRF_RADIO->MODE      = 0x00000000;      // 1 Mbps BLE mode

    // packet configuration:
    // S0LEN=0, LFLEN=0, S1LEN=0 — no header fields, fixed-size payload
    NRF_RADIO->PCNF0 = 0x00000000;

    // MAXLEN=22, STATLEN=22, BALEN=3, little-endian, no whitening
    // With LFLEN=0 the payload length = STATLEN = 22 bytes (fixed)
    NRF_RADIO->PCNF1 = 0x00031616;

    NRF_RADIO->RXADDRESSES = 0x00000001;
    NRF_RADIO->TXADDRESS   = 0x00000000;

    NRF_RADIO->CRCCNF = 0x00000000;        // no HW CRC (custom checksum)

    NRF_RADIO->FREQUENCY = 2UL;            // 2402 MHz
    NRF_RADIO->BASE0     = ((ADV_CHANNEL_ACCESS_ADDRESS) << 8)  & 0xFFFFFF00;
    NRF_RADIO->PREFIX0   = ((ADV_CHANNEL_ACCESS_ADDRESS) >> 24) & 0x000000FF;

    NRF_RADIO->SHORTS   = 0x00000000;       // no shortcuts — state managed manually
    NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;
    NVIC_EnableIRQ(RADIO_IRQn);
}


void rxEnableRADIO(void)
{
    uint16_t next_head = (rx_fifo_head + PACKET_SIZE) % RX_FIFO_SIZE;
    if (next_head != rx_fifo_tail)
    {
        /* FIFO has space: DMA directly into the next slot */
        NRF_RADIO->PACKETPTR = (uint32_t)&rx_fifo[rx_fifo_head];
        rx_dma_to_fifo = 1;
    }
    else
    {
        /* FIFO full: redirect DMA to discard buffer, packet is dropped */
        NRF_RADIO->PACKETPTR = (uint32_t)rx_discard;
        rx_dma_to_fifo = 0;
    }
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->EVENTS_END   = 0;
    NRF_RADIO->TASKS_RXEN   = 1;
    while (!NRF_RADIO->EVENTS_READY);
    NRF_RADIO->TASKS_START = 1;
}


/* Called from IRQ after END — radio is in RXIDLE, not DISABLED.
 * TASKS_RXEN only works from DISABLED; from RXIDLE we just need TASKS_START. */
static void rxRestartFromIdle(void)
{
    uint16_t next_head = (rx_fifo_head + PACKET_SIZE) % RX_FIFO_SIZE;
    if (next_head != rx_fifo_tail)
    {
        NRF_RADIO->PACKETPTR = (uint32_t)&rx_fifo[rx_fifo_head];
        rx_dma_to_fifo = 1;
    }
    else
    {
        NRF_RADIO->PACKETPTR = (uint32_t)rx_discard;
        rx_dma_to_fifo = 0;
    }
    NRF_RADIO->TASKS_START = 1;
}

void RADIO_IRQHandler(void)
{
    if (NRF_RADIO->EVENTS_END)
    {
        NRF_RADIO->EVENTS_END = 0;

        if (rx_dma_to_fifo)
            rx_fifo_head = (rx_fifo_head + PACKET_SIZE) % RX_FIFO_SIZE;

        rxRestartFromIdle();
    }
}


void startRADIO(void)
{
    initRADIO();
    rxEnableRADIO();
}


void txPktRADIO(const fet_packet_t *pkt)
{
    /* Prevent IRQ from restarting RX while we own the radio for TX */
    NVIC_DisableIRQ(RADIO_IRQn);

    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE   = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0);

    NRF_RADIO->PACKETPTR    = (uint32_t)pkt;
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_TXEN   = 1;
    while (NRF_RADIO->EVENTS_READY == 0);

    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->TASKS_START = 1;
    while (NRF_RADIO->EVENTS_END == 0);
    NRF_RADIO->EVENTS_END = 0;

    rxEnableRADIO();
    NVIC_EnableIRQ(RADIO_IRQn);
}


fet_packet_t *processRxFifoRADIO(void)
{
    while (rx_fifo_tail != rx_fifo_head)
    {
        uint16_t available = (rx_fifo_head - rx_fifo_tail + RX_FIFO_SIZE) % RX_FIFO_SIZE;
        if (available < PACKET_SIZE)
            break;

        fet_packet_t *pkt = (fet_packet_t *)&rx_fifo[rx_fifo_tail];
        rx_fifo_tail = (rx_fifo_tail + PACKET_SIZE) % RX_FIFO_SIZE;

        if (pkt->preamble != PREAMBLE_VALUE)    continue;
        if (!fet_packet_verify_checksum(pkt))   continue;

        return pkt;
    }

    return NULL;
}
