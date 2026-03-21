#ifndef MQTT_LINK_H
#define MQTT_LINK_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "hardware/adc.h"

#include "wifi_credentials.h"

enum class MqttStatus
{
    OK,
    TIMEOUT,
    ERREUR
};

class MQTTLink
{
public:
    MQTTLink();
    MqttStatus mqtt_connectTimeOut(ip_addr_t *broker_ip, int timeout_ms);
    MqttStatus mqtt_publishTimeOut(const char *topic, const void *msg_buffer, int timeout_ms);
    MqttStatus mqtt_subscribeTimeOut(const char *topic, int timeout_ms);
    bool init_wifi();
    void active_wait(uint32_t delay_ms);
};

#endif