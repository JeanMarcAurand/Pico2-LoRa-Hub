#include "SolarMeasureMsg.h"

SolarMeasureMsg::SolarMeasureMsg()
{
    memset(&_solarData, 0, sizeof(SolarData));
}
SolarMeasureMsg::~SolarMeasureMsg() {};

// --- Interface pour LoRa ---
uint8_t *SolarMeasureMsg::getLoRaPayload() const
{
    return ((uint8_t *)&_solarData);
}
void SolarMeasureMsg::updateWithLoRa(const uint8_t *payload)
{
    memcpy(&_solarData, payload, sizeof(SolarData));
}

// --- Interface pour MQTT ---
const std::string SolarMeasureMsg::getMqttTopic() const
{
    return (std::string("pico/measure/solar"));
}
const std::string SolarMeasureMsg::getMqttJson() const
{
    JsonDocument doc;

    doc["solar_current_raw"] = _solarData.iSolar;
    doc["battery_tension_raw"] = _solarData.vBat;
    doc["temperature_raw"] = _solarData.tempRaw;

    std::string output;
    serializeJson(doc, output);
    return (output);
}
void SolarMeasureMsg::updateWithMqtt(const char *json)
{
}

// --- Identification ---
uint8_t SolarMeasureMsg::getMessageTypeId() const
{
    return ((uint8_t)(LoRaMsgType::SOLAR_MEASURE));
}
