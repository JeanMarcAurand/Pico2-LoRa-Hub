#ifndef BOOT_REQUEST_MSG_H
#define BOOT_REQUEST_MSG_H

#include "ArduinoJson.h"

#include "BaseMessage.h"
#include "protocoleLora.h"

struct __attribute__((packed)) BootRequestData
{
    LoRaHeader header; // bits 0-39 (5 octets)
};

class BootRequestMsg : public BaseMessage
{
private:
    BootRequestData _bootRequestData;
    BootRequestMsg(/* args */);

public:
    ~BootRequestMsg();
    static BootRequestMsg *getInstance()
    {
        static BootRequestMsg instance; // Créée une seule fois ici.
        return &instance;
    }

    // --- Interface pour MQTT ---
    const char *getMqttSubscriptionTopic() const;
    const char *getMqttPublicationTopic() const;
    const std::string getMqttJson() const;
    void updateWithMqtt(const char *json);

    // --- Interface pour LoRa ---
    uint8_t *getLoRaPayload() const;
    void updateWithLoRa(const uint8_t *payload);

    // --- Identification ---
    uint8_t getMessageTypeId() const;

    // --- Affichage des valeurs ---
    void printLoRaValues(void) const;
};

#endif