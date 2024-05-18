//
// Created by kuresaru on 2024/1/14.
//
#include "keg.h"
#include "proto/mqtt-pub.pb-c.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <malloc.h>
#include <string.h>
#include <mosquitto.h>

struct mqtt_msg {
    struct mqtt_msg *next;
    char *topic;
    char *payload;
    bool retain;
};

typedef struct mqtt_msg mqtt_msg_t;

typedef struct {
    mqtt_msg_t *head;
    mqtt_msg_t *tail;
    pthread_t thread;
    pthread_mutex_t mutex;
} queue_t;

static queue_t q;
static struct mosquitto *mosq;

void mqtt_process_request(uint8_t *data, uint8_t len) {
    KegMqttPub *pub = keg_mqtt_pub__unpack(NULL, len, data);
    for (int i = 0; i < pub->n_item; i++) {
        KegMqttPubItem *item = pub->item[i];
        printf("pub %s %s\n", item->topic, item->payload);
        // copy msg
        mqtt_msg_t *msg = malloc(sizeof(mqtt_msg_t));
        msg->next = NULL;
        msg->topic = malloc(strlen(item->topic) + 1);
        strcpy(msg->topic, item->topic);
        msg->payload = malloc(strlen(item->payload) + 1);
        strcpy(msg->payload, item->payload);
        msg->retain = item->retain;
        // offer message
        pthread_mutex_lock(&q.mutex);
        if (q.tail == NULL) {
            q.tail = msg;
            q.head = msg;
        } else {
            q.tail->next = msg;
            q.tail = msg;
        }
        pthread_mutex_unlock(&q.mutex);
    }
    keg_mqtt_pub__free_unpacked(pub, NULL);
}

static void mqtt_publish(mqtt_msg_t *msg) {
    for (int i = 0; i < 3; i++) {
        int rc = mosquitto_publish(mosq, NULL, msg->topic, (int) strlen(msg->payload), msg->payload, 0, msg->retain);
        if (rc) {
            printf("failed to publish msg to mqtt: %d, reconnecting\n", rc);
            if (mosquitto_reconnect(mosq) != MOSQ_ERR_SUCCESS) {
                printf("failed to reconnect mqtt\n");
            }
        } else {
            return;
        }
    }
    printf("failed to publish msg\n");
}

_Noreturn static void *mqtt_process(void *param) {
    mqtt_msg_t msg;
    for (;;) {
        bool valid_flag = false;
        pthread_mutex_lock(&q.mutex);
        if (q.head != NULL) {
            valid_flag = true;
            memcpy(&msg, q.head, sizeof(mqtt_msg_t));
            q.head = q.head->next;
            if (q.head == NULL) {
                q.tail = NULL;
            }
        }
        pthread_mutex_unlock(&q.mutex);
        if (valid_flag) {
            mqtt_publish(&msg);
            free(msg.topic);
            free(msg.payload);
        } else {
            usleep(10000);
        }
        mosquitto_loop(mosq, 0, 1);
    }
}

void mqtt_start() {
    mosquitto_lib_init();
    mosq = mosquitto_new("ks-espnow-gateway", true, NULL);
    if (mosquitto_connect(mosq, "127.0.0.1", 1883, 60)) {
        printf("failed to connect mqtt\n");
    }

    memset(&q, 0, sizeof(queue_t));
    pthread_mutex_init(&q.mutex, NULL);
    pthread_create(&q.thread, NULL, mqtt_process, NULL);
}