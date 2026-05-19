#include "packet.h"


uint8_t fet_packet_compute_checksum(const fet_packet_t *pkt)
{
    const uint8_t *raw = (const uint8_t *)pkt;
    uint16_t packet_sum = 0;

    for (int i = 0; i < 21; i++) {
        packet_sum += raw[i];
    }

    return (uint8_t)(~packet_sum + 1);
}


int fet_packet_verify_checksum(const fet_packet_t *pkt)
{
    uint8_t computed_checksum = fet_packet_compute_checksum(pkt);
    return computed_checksum == pkt->checksum;
}


void fet_packet_build(fet_packet_t *pkt,
                      uint8_t daddr,
                      uint8_t saddr,
                      uint8_t cmd_id,
                      const uint8_t *payload,
                      uint8_t payload_size)
{
    pkt->preamble     = PREAMBLE_VALUE;
    pkt->daddr        = daddr;
    pkt->saddr        = saddr;
    pkt->cmd_id       = cmd_id;
    pkt->payload_size = payload_size;

    if (payload && payload_size > 0) {
        memcpy(pkt->payload, payload, payload_size);
    }

    for (int i = payload_size; i < MAX_PAYLOAD_SIZE; i++) {
        pkt->payload[i] = (uint8_t)(rand() & 0xFF);
    }

    pkt->checksum = fet_packet_compute_checksum(pkt);
}


void fet_packet_build_beacon(fet_packet_t *pkt,
                             uint8_t saddr,
                             const char *soc_name)
{
    uint8_t name_len = (uint8_t)strnlen(soc_name, MAX_PAYLOAD_SIZE);
    fet_packet_build(pkt, BROADCAST_ADDR, saddr,
                     FET_CMD_BEACON_IND,
                     (const uint8_t *)soc_name, name_len);
}


void fet_packet_build_msg_req(fet_packet_t *pkt,
                             uint8_t daddr,
                             uint8_t saddr,
                             const char *message)
{
    uint8_t msg_len = (uint8_t)strnlen(message, MAX_PAYLOAD_SIZE);
    fet_packet_build(pkt, daddr, saddr,
                     FET_CMD_MSG_TX_REQ,
                     (const uint8_t *)message, msg_len);
}


void fet_packet_build_msg_rep(fet_packet_t *pkt,
                             uint8_t daddr,
                             uint8_t saddr)
{
    fet_packet_build(pkt, daddr, saddr,
                     FET_CMD_MSG_TX_REP,
                     NULL, 0);
}