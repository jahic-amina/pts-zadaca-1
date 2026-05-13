#include <nrf52840.h>
#include <nrf52840_bitfields.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "rtc.h"
#include "uart.h"
#include "delay.h"
#include "radio.h"
#include "packet.h"

#define MY_ADDR 0x12
#define MY_NAME "nRF52-amina"

/* ------------------------------------------------------------------ */
/*  User list                                                           */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t addr;
    char name[MAX_PAYLOAD_SIZE + 1];
    uint32_t last_seen;
} user_info_t;

static volatile user_info_t users[10];

/* ------------------------------------------------------------------ */
/*  Retransmission state                                                */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t  active;
    uint8_t  daddr;
    char     msg[17];
    uint32_t sent_at_ms;
} retx_state_t;

static retx_state_t g_retx;

/* ------------------------------------------------------------------ */
/*  Forward declarations                                                */
/* ------------------------------------------------------------------ */
void initCLK(void);
void handleRxPktRADIO(const fet_packet_t *pkt);
void sendAck(const fet_packet_t *rx_pkt);
void sendMessage(uint8_t daddr, const char *msg);
void retxTask(void);
void beaconTask(void);

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    initCLK();
    initSYSTIM();

    initUART0(6, 8, UART0_BAUDRATE_115200);
    printUART0("\nwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n", 0);
    printUART0("w nRF52 iBeacon test...\n", 0);
    printUART0("-------------------------------------------------------\n", 0);
    deinitUART0(6, 8);

    initWakeupRTC1(1000);

    srand(MY_ADDR);

    while (1)
    {
        fet_packet_t *pkt = processRxFifoRADIO();
        if (pkt != NULL)
            handleRxPktRADIO(pkt);

        retxTask();
        beaconTask();
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Obrada primljenih paketa                                            */
/* ------------------------------------------------------------------ */
void handleRxPktRADIO(const fet_packet_t *pkt)
{
    switch (pkt->cmd_id)
    {
        case FET_CMD_BEACON_IND:
            for (int i = 0; i < 10; i++)
            {
                if (users[i].addr == 0)
                {
                    memcpy(users[i].name, (const char *)pkt->payload, pkt->payload_size);
                    users[i].name[pkt->payload_size] = '\0';
                    users[i].addr = pkt->saddr;
                    users[i].last_seen = getTIMEDATE();
                    break;
                }
            }
            break;

        case FET_CMD_MSG_TX_REQ:
            if (pkt->daddr != MY_ADDR && pkt->daddr != BROADCAST_ADDR)
                break;

            char msg[MAX_PAYLOAD_SIZE + 1];
            uint8_t len = pkt->payload_size;
            if (len > MAX_PAYLOAD_SIZE) len = MAX_PAYLOAD_SIZE;
            memcpy(msg, pkt->payload, len);
            msg[len] = '\0';

            printUART0("\n[MSG from 0x%xb]: %s\n", pkt->saddr, msg);
            sendAck(pkt);
            break;

        case FET_CMD_MSG_TX_REP:
            if (g_retx.active && pkt->saddr == g_retx.daddr)
            {
                g_retx.active = 0;
                printUART0("\n[ACK] Korisnik 0x%xb primio poruku.\n", g_retx.daddr);
            }
            break;

        default:
            break;
    }
}

/* ------------------------------------------------------------------ */
/*  Slanje ACK-a                                                        */
/* ------------------------------------------------------------------ */
void sendAck(const fet_packet_t *rx_pkt)
{
    fet_packet_t ack;
    fet_packet_build_msg_rep(&ack, rx_pkt->saddr, MY_ADDR);
    txPktRADIO(&ack);
}

/* ------------------------------------------------------------------ */
/*  Slanje poruke s fragmentacijom                                      */
/* ------------------------------------------------------------------ */
void sendMessage(uint8_t daddr, const char *msg)
{
    uint8_t total_len = (uint8_t)strlen(msg);
    uint8_t offset = 0;

    while (offset < total_len)
    {
        uint8_t frag_len = total_len - offset;
        if (frag_len > MAX_PAYLOAD_SIZE)
            frag_len = MAX_PAYLOAD_SIZE;

        fet_packet_t pkt;
        fet_packet_build(&pkt, daddr, MY_ADDR, FET_CMD_MSG_TX_REQ,
                         (const uint8_t *)(msg + offset), frag_len);
        txPktRADIO(&pkt);
        offset += frag_len;
    }

    g_retx.active     = 1;
    g_retx.daddr      = daddr;
    g_retx.sent_at_ms = getSYSTIM();
    strncpy(g_retx.msg, msg, 16);
    g_retx.msg[16] = '\0';
}

/* ------------------------------------------------------------------ */
/*  Retransmisijski task - poziva se iz main loopa                      */
/* ------------------------------------------------------------------ */
void retxTask(void)
{
    if (!g_retx.active)
        return;

    if (chk4TimeoutSYSTIM(g_retx.sent_at_ms, 3000) == SYSTIM_TIMEOUT)
    {
        printUART0("\n[RETX] Nema ACK-a, ponovo saljemo...\n", 0);
        sendMessage(g_retx.daddr, g_retx.msg);
    }
}

/* ------------------------------------------------------------------ */
/*  Beacon task - poziva se iz main loopa                               */
/* ------------------------------------------------------------------ */
void beaconTask(void)
{
    static uint32_t last_beacon_ms = 0;
    static uint32_t beacon_period  = 0;

    if (beacon_period == 0 ||
        chk4TimeoutSYSTIM(last_beacon_ms, beacon_period) == SYSTIM_TIMEOUT)
    {
        fet_packet_t beacon;
        fet_packet_build_beacon(&beacon, MY_ADDR, MY_NAME);
        txPktRADIO(&beacon);

        last_beacon_ms = getSYSTIM();
        beacon_period  = 1000 + (uint32_t)(rand() % 1001); /* rand(1000, 2000) ms */
    }
}

/* ------------------------------------------------------------------ */
/*  Clock init                                                          */
/* ------------------------------------------------------------------ */
void initCLK(void)
{
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

    NRF_CLOCK->LFCLKSRC = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0);
}