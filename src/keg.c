//
// Created by kuresaru on 2023/12/14.
//

#include "keg.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <hiredis/hiredis.h>

char *device_name;
char *rds_ip;
char *rds_port;
char *rds_dbn;

redisContext *rds_ctx;

void assert_rds(redisReply *reply, const char *msg) {
    if (reply->type == REDIS_REPLY_ERROR) {
        fprintf(stderr, "%s: %s\n", msg, reply->str);
        exit(1);
    }
}

static void parse_args(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "d:h:n:p:")) != -1) {
        switch (opt) {
            case 'd':
                device_name = optarg;
                break;
            case 'h':
                rds_ip = optarg;
                break;
            case 'p':
                rds_port = optarg;
                break;
            case 'n':
                rds_dbn = optarg;
        }
    }
}

static void load_encrypt() {
    redisReply *reply;
    reply = redisCommand(rds_ctx, "GET keg:encrypt:dh_prime");
    if ((reply->type != REDIS_REPLY_STRING) || (reply->len != 128)) {
        freeReplyObject(reply);
        uint8_t *dh_prime = malloc(128);
        assert(dh_prime != NULL);
        printf("generating dhparam\n");
        generate_dhparam(dh_prime);
        reply = redisCommand(rds_ctx, "SET keg:encrypt:dh_prime %b", dh_prime, 128);
        free(dh_prime);
        assert_rds(reply, "redis save dhparam error");
        freeReplyObject(reply);
        printf("generate dhparam done\n");
    } else {
        load_dhparam(reply->str);
        freeReplyObject(reply);
        printf("load dhparam from redis\n");
    }
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    printf("start options: device_name=%s rds=%s@%s:%s\n",
           device_name, rds_dbn, rds_ip, rds_port);
    if (device_name == NULL ||
        rds_ip == NULL ||
        rds_port == NULL ||
        rds_dbn == NULL) {
        printf("args error\n");
        return 1;
    }

    const struct timeval rds_timeout = {
            .tv_sec = 8000,
            .tv_usec = 0,
    };
    rds_ctx = redisConnectWithTimeout(rds_ip, atoi(rds_port), rds_timeout);
    if (rds_ctx->err) {
        printf("redis error: %s\n", rds_ctx->errstr);
        return 1;
    }

    redisReply *reply = redisCommand(rds_ctx, "SELECT %s", rds_dbn);
    assert_rds(reply, "select db failed");
    freeReplyObject(reply);

    load_encrypt();
    return 0;
}