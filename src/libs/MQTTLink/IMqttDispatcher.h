#ifndef I_MQTT_DISPATCHER_H
#define I_MQTT_DISPATCHER_H

#include <stdio.h>
#include "pico/stdlib.h"

class IMqttDispatcher {
public:
    virtual ~IMqttDispatcher() = default;
    
    // Appelés par MQTTLink quand lwIP bouge
    virtual void dispatchPublish(const char* topic, uint32_t tot_len) = 0;
    virtual void dispatchData(const uint8_t* data, uint16_t len, uint8_t flags) = 0;
};

#endif