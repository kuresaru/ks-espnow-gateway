//
// Created by kuresaru on 2023/12/14.
//

#ifndef KS_ESPNOW_GATEWAY_KEG_H
#define KS_ESPNOW_GATEWAY_KEG_H

#include <stdint.h>
#include <protobuf-c/protobuf-c.h>

#define KEG_TRX_TYPE_DATA 0x02
#define KEG_TRX_TYPE_ACK 0x03
#define KEG_TRX_TYPE_MQTT_PUB 0x04

typedef struct {
    uint32_t seq;
    uint8_t type;
    uint8_t len;
    uint8_t data[];
} __attribute__((__packed__)) keg_pkt_t;

void mqtt_start();
void mqtt_process_request(uint8_t *data, uint8_t len);

        void encrypt_aes(const uint8_t *key, const uint8_t *in, int in_len, uint8_t *out, int *out_len);
void decrypt_aes(const uint8_t *key, const uint8_t *in, int in_len, uint8_t *out, int *out_len);
void calculate_md5(uint8_t *md5, const uint8_t *key, const uint8_t *data, int len);

_Noreturn void transport_start(char *device_name);

#endif //KS_ESPNOW_GATEWAY_KEG_H
