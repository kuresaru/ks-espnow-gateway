//
// Created by kuresaru on 2023/12/14.
//

#include <stdio.h>
#include <openssl/dh.h>

DH *dh;

void generate_dhparam(uint8_t *dh_prime) {
    dh = DH_new();
    DH_generate_parameters_ex(dh, 1024, DH_GENERATOR_2, NULL);
    if (DH_generate_key(dh) <= 0) {
        fprintf(stderr, "dh genkey err\n");
        exit(1);
    }
    const BIGNUM *bn_dh_p = DH_get0_p(dh);
    BN_bn2bin(bn_dh_p, dh_prime);
}

void load_dhparam(uint8_t *dh_prime) {
    const uint8_t dh_generator = DH_GENERATOR_2;
    DH *dh = DH_new();
    BIGNUM *bn_dh_p, *bn_dh_g;
    bn_dh_p = BN_bin2bn(dh_prime, 128, NULL);
    bn_dh_g = BN_bin2bn(&dh_generator, 1, NULL);
    if ((!bn_dh_p) || (!bn_dh_g)) {
        fprintf(stderr, "load dh p&g err\n");
        exit(1);
    }
    DH_set0_pqg(dh, bn_dh_p, NULL, bn_dh_g);
    if (DH_generate_key(dh) <= 0) {
        fprintf(stderr, "dh genkey err\n");
        exit(1);
    }
}

void dh_get_and_exchange(
        uint8_t **serverkey_buf, uint32_t *serverkey_len,
        uint8_t **exkey_buf, uint32_t *exkey_len,
        const uint8_t *clientkey, uint32_t clientkey_len) {
    BIGNUM *bn_client_pubkey;
    const BIGNUM *bn_server_pubkey;
    DH_get0_key(dh, &bn_server_pubkey, NULL);
    *serverkey_buf = malloc(BN_num_bytes(bn_server_pubkey));
    *serverkey_len = BN_bn2bin(bn_server_pubkey, *serverkey_buf);

    bn_client_pubkey = BN_bin2bn(clientkey, clientkey_len, NULL);
    *exkey_buf = malloc(DH_size(dh));
    *exkey_len = DH_compute_key(*exkey_buf, bn_client_pubkey, dh);
}