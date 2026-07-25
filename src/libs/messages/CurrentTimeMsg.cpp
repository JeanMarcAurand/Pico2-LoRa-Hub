#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "CurrentTimeMsg.h"

CurrentTimeMsg::CurrentTimeMsg()
{
    memset(&_CurrentTimeData, 0, sizeof(CurrentTimeData));
}
CurrentTimeMsg::~CurrentTimeMsg() {};

// --- Interface pour LoRa ---
uint8_t *CurrentTimeMsg::getLoRaPayload() const
{
    return ((uint8_t *)&_CurrentTimeData);
}
void CurrentTimeMsg::updateWithLoRa(const uint8_t *payload)
{
    memcpy(&_CurrentTimeData, payload, sizeof(CurrentTimeData));
}

// --- Interface pour MQTT ---
const char *CurrentTimeMsg::getMqttPublicationTopic() const
{
    return (nullptr);
}
const char *CurrentTimeMsg::getMqttSubscriptionTopic() const
{
    return ("pico/clock/currentTime");
}
const std::string CurrentTimeMsg::getMqttJson() const
{
    JsonDocument doc;

    JsonObject current_time = doc["current_time"].to<JsonObject>();
    current_time["heure"] = _CurrentTimeData.heure;
    current_time["minute"] = _CurrentTimeData.minute;
    current_time["seconde"] = _CurrentTimeData.seconde;
    current_time["annee"] = _CurrentTimeData.annee;
    current_time["mois"] = _CurrentTimeData.mois;
    current_time["jour"] = _CurrentTimeData.jour;
    current_time["jourDeLaSemaine"] = _CurrentTimeData.jourSemaine;

    std::string output;
    serializeJson(doc, output);
    return (output);
}
void CurrentTimeMsg::updateWithMqtt(const char *json)
{
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (!err)
    {
        JsonObject current_time = doc["current_time"];
        _CurrentTimeData.heure = current_time["heure"] | 0;
        _CurrentTimeData.minute = current_time["minute"] | 0;
        _CurrentTimeData.seconde = current_time["seconde"] | 0;
        _CurrentTimeData.annee = current_time["annee"] | 0;
        _CurrentTimeData.mois = current_time["mois"] | 0;
        _CurrentTimeData.jour = current_time["jour"] | 0;
        _CurrentTimeData.jourSemaine = current_time["jourDeLaSemaine"] | 0;
    }
}
// --- Identification ---
uint8_t CurrentTimeMsg::getMessageTypeId() const
{
    return ((uint8_t)(LoRaMsgType::CURRENT_TIME));
}

// --- Affichage des valeurs ---
void CurrentTimeMsg::printLoRaValues(void) const
{
    printf(">-- CurrentTimeMsg id = %d ---\n", getMessageTypeId());
    printLoraHeader(&(_CurrentTimeData.header));
    printf("--- Data ---\n");
    printf(" -- Heure Courante --\n");
    printf("  heure:   %d\n", _CurrentTimeData.heure);
    printf("  minute:   %d\n", _CurrentTimeData.minute);
    printf("  seconde: %d\n", _CurrentTimeData.seconde);
    printf("  annee: %d\n", _CurrentTimeData.annee);
    printf("  mois:  %d\n", _CurrentTimeData.mois);
    printf("  jour:  %d\n", _CurrentTimeData.jour);
    printf("  jour de la semaine: %d\n", _CurrentTimeData.jourSemaine);
    printf("--- CurrentTimeMsg --<\n");
}