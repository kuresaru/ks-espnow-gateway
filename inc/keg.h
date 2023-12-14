//
// Created by kuresaru on 2023/12/14.
//

#ifndef KS_ESPNOW_GATEWAY_KEG_H
#define KS_ESPNOW_GATEWAY_KEG_H

#include <stdint.h>

typedef struct {
    uint8_t type;
    uint8_t reserved[3];
    uint8_t data[];
} keg_raw_pkt_t;

void generate_dhparam(uint8_t *dh_prime);

void load_dhparam(uint8_t *dh_prime);

void dh_get_and_exchange(uint8_t **serverkey_buf, uint32_t *serverkey_len, uint8_t **exkey_buf, uint32_t *exkey_len,
                         const uint8_t *clientkey, uint32_t clientkey_len);

#endif //KS_ESPNOW_GATEWAY_KEG_H
