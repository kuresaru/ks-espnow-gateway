//
// Created by kuresaru on 2023/12/18.
//

#include "keg.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <hiredis/hiredis.h>
#include <arpa/inet.h>
#include "libesp-now_linux.h"

extern redisContext *rds_ctx;
static int fd_send, fd_recv;

static void get_mac(char *dev, uint8_t *mac) {
    FILE *fp;
    char *path;
    char mac_str[18];
    unsigned int mt[6];
    int i;

    path = malloc(strlen(dev) + 24);
    sprintf(path, "/sys/class/net/%s/address", dev);

    fp = fopen(path, "r");
    assert(fp != NULL);
    fread(mac_str, 1, 17, fp);
    mac_str[17] = '\0';
    fclose(fp);
    free(path);

    sscanf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", &mt[0], &mt[1], &mt[2], &mt[3], &mt[4], &mt[5]);
    for (i = 0; i < 6; i++) {
        mac[i] = mt[i];
    }
}

static void encrypt_and_send(const uint8_t *dest_mac, const uint8_t *key, keg_pkt_t *pkt, uint8_t pkt_len) {
    int send_buf_len = pkt_len + (16 - (pkt_len % 16)) + 32;
    uint8_t *send_buf = malloc(send_buf_len);
    int olen;
    calculate_md5(send_buf, key, (uint8_t *) pkt, pkt_len);
    encrypt_aes(key, (uint8_t *) pkt, pkt_len, send_buf + 16, &olen);
    assert(olen + 16 == send_buf_len);
    esp_now_send(fd_send, dest_mac, send_buf, send_buf_len);
    free(send_buf);
}

static void process_recv_pkt(char *from_str, uint8_t *from, uint8_t *buf, uint8_t len) {
    uint8_t md5[16];
    uint8_t key[16];
    uint32_t acked_seq;

    // get client info
    redisReply *i_key, *i_val;
    redisReply *reply = redisCommand(rds_ctx, "HGETALL keg:client:%s", from_str);
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (int i = 0; i < reply->elements; i += 2) {
            i_key = reply->element[i];
            i_val = reply->element[i + 1];
            if (!strcmp(i_key->str, "key") && i_val->len == 16) {
                memcpy(key, i_val->str, 16);
            } else if (!strcmp(i_key->str, "seq")) {
                acked_seq = atoi(i_val->str);
            }
        }
    }
    freeReplyObject(reply);

    // decrypt packet
    uint8_t *decrypted = malloc(len - 16);
    int decrypted_len;
    decrypt_aes(key, buf + 16, len - 16, decrypted, &decrypted_len);
    if (decrypted_len > 0) {
        // process packet
        calculate_md5(md5, key, decrypted, decrypted_len);
        if (memcmp(buf, md5, 16) == 0) {
            keg_pkt_t *response;
            int response_len;
            bool normal_ack = false;
            keg_pkt_t *pkt = (keg_pkt_t *) decrypted;
            pkt->seq = ntohl(pkt->seq);
            if ((pkt->seq == 0) && (pkt->type == KEG_TRX_TYPE_DATA)) {
                // request query seq
                printf("query seq from %s\n", from_str);
                response_len = sizeof(keg_pkt_t) + 4;
                response = malloc(response_len);
                response->seq = 0;
                response->type = KEG_TRX_TYPE_ACK;
                response->len = 4;
                *((uint32_t *)response->data) = htonl(acked_seq);
                encrypt_and_send(from, key, response, response_len);
                free(response);
            } else {
                if (acked_seq + 1 == pkt->seq) {
                    // process pkt
                    if (pkt->type == KEG_TRX_TYPE_MQTT_PUB) {
                        // read mqtt pub
                        mqtt_process_request(pkt->data, pkt->len);
                        normal_ack = true;
                    }
                    // incr seq
                    reply = redisCommand(rds_ctx, "HINCRBY keg:client:%s seq 1", from_str);
                    assert(reply->type == REDIS_REPLY_INTEGER);
                    printf("seq %s incr to %lld\n", from_str, reply->integer);
                    freeReplyObject(reply);
                } else {
                    // send dump ack
                    normal_ack = true;
                }
                if (normal_ack) {
                    response_len = sizeof(keg_pkt_t);
                    response = malloc(response_len);
                    response->seq = htonl(pkt->seq);
                    response->type = KEG_TRX_TYPE_ACK;
                    response->len = 0;
                    encrypt_and_send(from, key, response, response_len);
                    free(response);
                }
            }
        }
    }
    free(decrypted);
}

_Noreturn void transport_start(char *device_name) {
    uint8_t mac[6];

    get_mac(device_name, mac);
    printf("my mac %02x%02x%02x%02x%02x%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    create_raw_socket(device_name, &fd_send, &fd_recv);
    assert(fd_send != -1 && fd_recv != -1);
    esp_now_init(mac);

    char from_str[13];
    uint8_t *from;
    uint8_t *recvbuf;
    uint8_t recvlen;

    for (;;) {
        esp_now_recv(fd_recv, &recvbuf, &recvlen, &from);
        sprintf(from_str, "%02x%02x%02x%02x%02x%02x", from[0], from[1], from[2], from[3], from[4], from[5]);

        printf("recv from %s len=%d\n", from_str, recvlen);
        for (uint8_t i = 0; i < recvlen; i++) {
            printf("%02X ", recvbuf[i]);
        }
        printf("\n");

        if (recvlen >= 48) {
            process_recv_pkt(from_str, from, recvbuf, recvlen);
        }
    }
}