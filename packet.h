#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define PREAMBLE_VALUE   0x5A
#define PACKET_SIZE      22
#define MAX_PAYLOAD_SIZE 16
#define BROADCAST_ADDR   0xFF

typedef struct {
    uint8_t preamble;               
    uint8_t daddr;
    uint8_t saddr;
    uint8_t cmd_id;    
    uint8_t payload_size;           
    uint8_t payload[MAX_PAYLOAD_SIZE]; 
    uint8_t checksum;               
} __attribute__((packed)) fet_packet_t;

typedef enum {
    FET_CMD_BEACON_IND  = 0x01,
    FET_CMD_MSG_TX_REQ  = 0x02,
    FET_CMD_MSG_TX_REP  = 0x03,
} fet_cmd_id_t;

uint8_t fet_packet_compute_checksum(const fet_packet_t *pkt);
int     fet_packet_verify_checksum(const fet_packet_t *pkt);
void    fet_packet_build(fet_packet_t *pkt,
                         uint8_t daddr,
                         uint8_t saddr,
                         uint8_t cmd_id,
                         const uint8_t *payload,
                         uint8_t payload_size);

void    fet_packet_build_beacon(fet_packet_t *pkt,
                                uint8_t saddr,
                                const char *soc_name);

void    fet_packet_build_msg_req(fet_packet_t *pkt,
                                 uint8_t daddr,
                                 uint8_t saddr,
                                 const char *message);

void    fet_packet_build_msg_rep(fet_packet_t *pkt,
                                 uint8_t daddr,
                                 uint8_t saddr);