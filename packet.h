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
    uint16_t payload[MAX_PAYLOAD_SIZE]; 
    uint8_t checksum;               
} __attribute__((packed)) fet_packet_t;

typedef enum {
    FET_CMD_BEACON_IND  = 0x01,
    FET_CMD_MSG_TX_REQ  = 0x02,
    FET_CMD_MSG_TX_REP  = 0x03,
} fet_cmd_id_t;