#ifndef SOLAR_MEASURE_MSG_H
#define SOLAR_MEASURE_MSG_H

#include "ArduinoJson.h"

#include "BaseMessage.h"
#include "protocoleLora.h"

/**
 * Message spécifique au capteur de rayonnement solaire
 */
struct __attribute__((packed)) SolarData
{
    LoRaHeader header; // 40 bits (5 octets)

    uint32_t iSolar : 12;  // Courant ADC (0-4095).
    uint32_t vBat : 10;    // Tension batterie (précision de ~4mV pour une plage 0-4V).
    uint32_t tempRaw : 10; // Température (Raw = (T*10)+400 . Offset de 400 pour gérer de -40°C à +62.3°C).
};

class SolarMeasureMsg : public BaseMessage
{
private:
    SolarData _solarData;

public:
    SolarMeasureMsg(/* args */);
    ~SolarMeasureMsg();

    // --- Interface pour MQTT ---
    const std::string getMqttTopic() const;
    const std::string getMqttJson() const;
    void updateWithMqtt(const char *json);

    // --- Interface pour LoRa ---
    uint8_t *getLoRaPayload() const;
    void updateWithLoRa(const uint8_t *payload);

    // --- Identification ---
    uint8_t getMessageTypeId() const;
};
#endif