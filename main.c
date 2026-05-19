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

#define MY_ADDR 0x02
#define MY_NAME "nRF52-azur"

//Druga plocica
//#define MY_ADDR 0x02
//#define MY_NAME "nRF52-azur"

/* ------------------------------------------------------------------ */
/*  User list                                                           */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t addr;
    char name[MAX_PAYLOAD_SIZE + 1];
    uint32_t last_seen;
} user_info_t;

static user_info_t users[10];

/* ------------------------------------------------------------------ */
/*  Retransmission state                                                */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t  active;
    uint8_t  retx_count;    /* 0 = not yet retransmitted, 1 = already retransmitted once */
    uint8_t  daddr;
    char     msg[257];
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
void userListTask(void);
void cliTask(void);
void parseCLI(const char *line);

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    initCLK();
    initSYSTIM();

    initUART0(6, 8, UART0_BAUDRATE_115200);
    printUART0("\nwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n", 0);
    printUART0("w nRF52 PTS Zadaca 1...\n", 0);
    printUART0("-------------------------------------------------------\n", 0);

    initWakeupRTC1(1000);

    srand(MY_ADDR);

    startRADIO();

    while (1)
    {
        fet_packet_t *pkt = processRxFifoRADIO();
        if (pkt != NULL)
            handleRxPktRADIO(pkt);

        retxTask();
        beaconTask();
        userListTask();
        cliTask();
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
        {
            /* Traži postojećeg korisnika */
            int found = -1;
            for (int i = 0; i < 10; i++)
            {
                if (users[i].addr == pkt->saddr)
                {
                    found = i;
                    break;
                }
            }

            if (found >= 0)
            {
                /* Ažuriraj last_seen za postojećeg korisnika */
                users[found].last_seen = getTIMEDATE();
            }
            else
            {
                /* Dodaj novog korisnika na prvo slobodno mjesto */
                for (int i = 0; i < 10; i++)
                {
                    if (users[i].addr == 0)
                    {
                        uint8_t len = pkt->payload_size;
                        if (len > MAX_PAYLOAD_SIZE) len = MAX_PAYLOAD_SIZE;
                        memcpy(users[i].name, (const char *)pkt->payload, len);
                        users[i].name[len] = '\0';
                        users[i].addr      = pkt->saddr;
                        users[i].last_seen = getTIMEDATE();
                        break;
                    }
                }
            }
            break;
        }

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
    g_retx.retx_count = 0;
    g_retx.daddr      = daddr;
    g_retx.sent_at_ms = getSYSTIM();
    strncpy(g_retx.msg, msg, 256);
    g_retx.msg[256] = '\0';
}

/* ------------------------------------------------------------------ */
/*  Retransmisijski task - poziva se iz main loopa                      */
/* ------------------------------------------------------------------ */
void retxTask(void)
{
    if (!g_retx.active)
        return;

    if (chk4TimeoutSYSTIM(g_retx.sent_at_ms, 3000) != SYSTIM_TIMEOUT)
        return;

    if (g_retx.retx_count == 0)
    {
        /* First (and only) retransmission */
        g_retx.retx_count = 1;
        g_retx.sent_at_ms = getSYSTIM();
        printUART0("\n[RETX] Nema ACK-a, ponovo saljemo...\n", 0);

        uint8_t total_len = (uint8_t)strlen(g_retx.msg);
        uint8_t offset    = 0;
        while (offset < total_len)
        {
            uint8_t frag_len = total_len - offset;
            if (frag_len > MAX_PAYLOAD_SIZE) frag_len = MAX_PAYLOAD_SIZE;
            fet_packet_t pkt;
            fet_packet_build(&pkt, g_retx.daddr, MY_ADDR, FET_CMD_MSG_TX_REQ,
                             (const uint8_t *)(g_retx.msg + offset), frag_len);
            txPktRADIO(&pkt);
            offset += frag_len;
        }
    }
    else
    {
        /* Already retransmitted once — give up */
        g_retx.active = 0;
        printUART0("\n[RETX] Nema potvrde, odustajemo.\n", 0);
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
/*  User list task - periodični ispis + brisanje korisnika nakon 10s  */
/* ------------------------------------------------------------------ */
void userListTask(void)
{
    static uint32_t last_print_s = 0;
    uint32_t now_s = getTIMEDATE();

    /* Ukloni korisnike koji nisu viđeni 10 sekundi */
    for (int i = 0; i < 10; i++)
    {
        if (users[i].addr == 0)
            continue;
        if (chk4TimeoutTIMEDATE(users[i].last_seen, 10) == RTC_TIMEOUT)
        {
            printUART0("\n[USER] Korisnik %s (0x%xb) uklonjen (timeout).\n",
                       users[i].name, users[i].addr);
            users[i].addr = 0;
            users[i].name[0] = '\0';
            users[i].last_seen = 0;
        }
    }

    /* Ispiši listu svakih 5 sekundi */
    if (chk4TimeoutTIMEDATE(last_print_s, 5) == RTC_TIMEOUT)
    {
        last_print_s = now_s;
        printUART0("\n--- Lista korisnika ---\n", 0);
        int count = 0;
        for (int i = 0; i < 10; i++)
        {
            if (users[i].addr != 0)
            {
                printUART0("  [0x%xb] %s\n", users[i].addr, users[i].name);
                count++;
            }
        }
        if (count == 0)
            printUART0("  (nema korisnika)\n", 0);
        printUART0("-----------------------\n", 0);
        printUART0("> ", 0);
    }
}

/* ------------------------------------------------------------------ */
/*  CLI - unos destinacije i poruke                                     */
/* ------------------------------------------------------------------ */
void parseCLI(const char *line)
{
    /* Expected format: "<hex_addr> <message>"  e.g. "AB Hello world" */
    const char *space = strchr(line, ' ');
    if (space == NULL)
    {
        printUART0("[CLI] Format: <hex_adresa> <poruka>\n", 0);
        return;
    }
    uint8_t daddr = (uint8_t)strtol(line, NULL, 16);
    const char *msg = space + 1;
    if (*msg == '\0')
    {
        printUART0("[CLI] Poruka je prazna.\n", 0);
        return;
    }
    printUART0("\n[CLI] Saljem -> 0x%xb: %s\n", daddr, msg);
    sendMessage(daddr, msg);
}

void cliTask(void)
{
    static char    cli_buf[257];
    static uint8_t cli_len      = 0;
    static uint8_t prompt_shown = 0;

    if (!prompt_shown)
    {
        printUART0("\n> ", 0);
        prompt_shown = 1;
    }

    uint8_t ch;
    while (getCharUART0(&ch))
    {
        if (ch == '\r' || ch == '\n')
        {
            cli_buf[cli_len] = '\0';
            printUART0("\n", 0);
            if (cli_len > 0)
                parseCLI(cli_buf);
            cli_len      = 0;
            prompt_shown = 0;
        }
        else if ((ch == 0x08 || ch == 0x7F) && cli_len > 0)
        {
            /* Backspace: erase last character on terminal */
            cli_len--;
            putcharUART0(0x08);
            putcharUART0(' ');
            putcharUART0(0x08);
        }
        else if (ch >= 0x20 && cli_len < 256)
        {
            cli_buf[cli_len++] = ch;
            putcharUART0(ch);   /* echo */
        }
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