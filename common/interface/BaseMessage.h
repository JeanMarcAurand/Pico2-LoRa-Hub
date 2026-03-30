#ifndef BASE_MESSAGE_H
#define BASE_MESSAGE_H

#include <cstdint>

class BaseMessage {
public:
    virtual ~BaseMessage() {}

    // --- Interface pour MQTT ---
    virtual const std::string getMqttTopic() const = 0;
    virtual const std::string getMqttJson() const = 0;
    virtual void updateWithMqtt(const char* json) = 0;

    // --- Interface pour LoRa ---
    virtual uint8_t* getLoRaPayload() const = 0;
    virtual void updateWithLoRa(const uint8_t* payload) = 0;

    // --- Identification ---
   virtual uint8_t getMessageTypeId() const = 0;
};
#endif