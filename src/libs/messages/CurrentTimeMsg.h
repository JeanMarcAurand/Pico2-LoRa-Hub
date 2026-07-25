#ifndef CURRENT_TIME_MSG_H
#define CURRENT_TIME_MSG_H

#include "ArduinoJson.h"

#include "BaseMessage.h"
#include "protocoleLora.h"

struct __attribute__((packed)) CurrentTimeData
{
    // --- Header LoRa (5 octets / 40 bits) ---
    LoRaHeader header;

    // --- Payload Temporel (5 octets / 40 bits) ---
    // Heure : 0 à 23 ( 2^5 = 32).
    uint32_t heure : 5;
    // Minutes : 0 à 59 ( 2^6 = 64).
    uint32_t minute : 6;
    // Secondes : 0 à 59 .
    uint32_t seconde : 6;
    // Année : Offset par rapport à l'an 2000 (0 à 127).
    // Permet de tenir jusqu'en 2127 :-)) ( 2^7 = 128).
    uint32_t annee : 7;
    // Mois : 0 (Janvier) à 11 (Décembre) (2^4 = 16).
    uint32_t mois : 4;
    // Jour : 1 à 31 (2^5 = 32).
    uint32_t jour : 5;
    // Jour de la semaine : 0 (Dimanche) à 6 (Samedi) (2^3 = 8).
    uint32_t jourSemaine : 3;
    // Padding : Pour arriver à un alignement parfait sur des octets complets (40 bits),
    // on ajoute 4 bits de "remplissage".
    uint32_t padding : 4;
};

class CurrentTimeMsg : public BaseMessage
{
private:
    CurrentTimeData _CurrentTimeData;
    std::string _CurrentTimeDataJson;

    CurrentTimeMsg(/* args */);

public:
    ~CurrentTimeMsg();
    static CurrentTimeMsg *getInstance()
    {
        static CurrentTimeMsg instance; // Créée une seule fois ici.
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