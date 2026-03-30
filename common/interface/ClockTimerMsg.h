#ifndef CLOCK_TIMER_MSG_H
#define CLOCK_TIMER_MSG_H

#include "ArduinoJson.h"

#include "BaseMessage.h"
#include "protocoleLora.h"

enum class ClockTimerAlarm
{
    ARRET,     // Pas d'alarme.
    UNE_FOIS,  // Prendre encompte l'amarme une seule fois.
    RECCURENT, // Répéter l'alame sans fin
    FORCE      // Force l'alarme.
};
struct __attribute__((packed)) ClockTimerData
{
    LoRaHeader header; // LoRaHeader (40 bits)

    // Bloc Heure Courante (17 bits)
    uint32_t heure : 5;   // bits 41-45
    uint32_t minute : 6;  // bits 46-51
    uint32_t seconde : 6; // bits 52-57

    // Mode Alarme (2 bits)
    uint32_t alarmMode : 2; // bits 58-59

    // Bloc Heure Alarme (17 bits)
    uint32_t hAlarme : 5; // bits 60-64
    uint32_t mAlarme : 6; // bits 65-70
    uint32_t sAlarme : 6; // bits 71-76

    // Durée (11 bits)
    uint32_t duree : 11; // bits 77-87

    uint32_t padding : 1; // bit 88 -> Aligné sur 11 octets pile !
};

class ClockTimerMsg : public BaseMessage
{
private:
    ClockTimerData _clockTimerData;

public:
    ClockTimerMsg(/* args */);
    ~ClockTimerMsg();

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