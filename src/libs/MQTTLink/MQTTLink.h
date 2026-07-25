#ifndef MQTT_LINK_H
#define MQTT_LINK_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"

#include "IMqttDispatcher.h"

#include "../../config/wifi_credentials.h"

enum class MqttStatus
{
    OK,
    TIMEOUT,
    ERREUR
};

class MQTTLink
{
private:
    IMqttDispatcher *dispatcher = nullptr;
    mqtt_client_t *static_client = nullptr;

public:
    MQTTLink();
    void setDispatcher(IMqttDispatcher *newDispatcher)
    {
        dispatcher = newDispatcher;
    }

    bool mqtt_is_connected = false; // Flag de confirmation.
    MqttStatus mqtt_connectTimeOut(ip_addr_t *broker_ip, int timeout_ms);

    bool publish_in_progress = false; // Flag de confirmation.
    MqttStatus mqtt_publishTimeOut(const char *topic, const void *msg_buffer, int timeout_ms);

    bool mqtt_sub_ready = false; // Flag de confirmation.
    MqttStatus mqtt_subscribeTimeOut(const char *topic, int timeout_ms);

    bool init_wifi();
    void active_wait(uint32_t delay_ms);
};

#endif