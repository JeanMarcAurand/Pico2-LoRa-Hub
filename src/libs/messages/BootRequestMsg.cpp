#include "BootRequestMsg.h"

BootRequestMsg::BootRequestMsg()
{
    memset(&_bootRequestData, 0, sizeof(BootRequestData));
}
BootRequestMsg::~BootRequestMsg() {};

// --- Interface pour LoRa ---
uint8_t *BootRequestMsg::getLoRaPayload() const
{
    return ((uint8_t *)&_bootRequestData);
}
void BootRequestMsg::updateWithLoRa(const uint8_t *payload)
{
    memcpy(&_bootRequestData, payload, sizeof(BootRequestData));
}

// --- Interface pour MQTT ---
const char *BootRequestMsg::getMqttPublicationTopic() const
{
    return ("pico/bootRequest");
}
const char *BootRequestMsg::getMqttSubscriptionTopic() const
{
    return (nullptr);
}

const std::string BootRequestMsg::getMqttJson() const
{
    JsonDocument doc;

    JsonObject boot = doc["boot"].to<JsonObject>();
    boot["nodeId"] = (int)_bootRequestData.header.srcNodeID;

    std::string output;
    serializeJson(doc, output);
    return (output);
}
void BootRequestMsg::updateWithMqtt(const char *json)
{
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (!err)
    {
        JsonObject boot = doc["boot"];
        _bootRequestData.header.srcNodeID = (LoRaNodeIdType)(boot["nodeId"] | 0);
    }
}
// --- Identification ---
uint8_t BootRequestMsg::getMessageTypeId() const
{
    return ((uint8_t)(LoRaMsgType::CLOCK_TIMER));
}

// --- Affichage des valeurs ---
void BootRequestMsg::printLoRaValues(void) const
{
    printf(">-- BootRequestMsg id = %d ---\n", getMessageTypeId());
    printLoraHeader(&(_bootRequestData.header));
    printf("--- BootRequestMsg --<\n");
}