#ifndef CLOCK_TIMER_MSG_H
#define CLOCK_TIMER_MSG_H

#include "ArduinoJson.h"

#include "BaseMessage.h"
#include "protocoleLora.h"

enum class ClockTimerAlarm : uint8_t // Préciser le type pour contrôler la taille
{
    ARRET = 0,        // Désactive totalement l'alarme
    UNE_FOIS = 1,     // Se déclenche une fois, puis passe à ARRET
    RECURRENT = 2,    // Se déclenche toutes les 24h
    FORCE = 3,        // Allumage permanent (ignore l'heure)
    FORCE_TIMEOUT = 4 // Allumage immédiat pendant la durée spécifiée
};

struct __attribute__((packed)) ClockTimerData
{
    LoRaHeader header; // bits 0-39 (5 octets)

    // Mode Alarme.
    uint32_t alarmMode : 3; // bits 40-42

    // Bloc Heure Alarme (Heure de début)
    uint32_t hAlarme : 5; // bits 43-47 (0-31)
    uint32_t mAlarme : 6; // bits 48-53 (0-63)
    uint32_t sAlarme : 6; // bits 54-59 (0-63)

    // Durée en secondes (17 bits pour atteindre 24h=86400s).
    uint32_t duree : 17; // bits 60-76

    uint32_t padding : 3; // bits 77-79 -> Total 80 bits = 10 octets
};

class ClockTimerMsg : public BaseMessage
{
private:
    ClockTimerData _clockTimerData;
    ClockTimerMsg(/* args */);

public:
    ~ClockTimerMsg();
    static ClockTimerMsg *getInstance()
    {
        static ClockTimerMsg instance; // Créée une seule fois ici.
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