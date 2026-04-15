#include "ClockTimerMsg.h"

ClockTimerMsg::ClockTimerMsg()
{
    memset(&_clockTimerData, 0, sizeof(ClockTimerData));
}
ClockTimerMsg::~ClockTimerMsg() {};

// --- Interface pour LoRa ---
uint8_t *ClockTimerMsg::getLoRaPayload() const
{
    return ((uint8_t *)&_clockTimerData);
}
void ClockTimerMsg::updateWithLoRa(const uint8_t *payload)
{
    memcpy(&_clockTimerData, payload, sizeof(ClockTimerData));
}

// --- Interface pour MQTT ---
const std::string ClockTimerMsg::getMqttTopic() const
{
    return (std::string("pico/clock/control"));
}
const std::string ClockTimerMsg::getMqttJson() const
{
    JsonDocument doc;

    // On crée une belle structure imbriquée pour le PC
    JsonObject current_time = doc["current_time"].to<JsonObject>();
    current_time["h"] = _clockTimerData.heure;
    current_time["m"] = _clockTimerData.minute;
    current_time["s"] = _clockTimerData.seconde;

    JsonObject alarm = doc["alarm"].to<JsonObject>();
    alarm["mode"] = (int)_clockTimerData.alarmMode;
    alarm["h"] = _clockTimerData.hAlarme;
    alarm["m"] = _clockTimerData.mAlarme;
    alarm["duration"] = _clockTimerData.duree;

    std::string output;
    serializeJson(doc, output);
    return (output);
}
void ClockTimerMsg::updateWithMqtt(const char *json)
{
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (!err)
    {
        JsonObject current_time = doc["current_time"];
        _clockTimerData.heure = current_time["h"] | 0;
        _clockTimerData.minute = current_time["m"] | 0;
        _clockTimerData.seconde = current_time["s"] | 0;

        JsonObject alarm = doc["alarm"];
        _clockTimerData.alarmMode = alarm["mode"] | 0;
        _clockTimerData.hAlarme = alarm["h"] | 0;
        _clockTimerData.mAlarme = alarm["m"] | 0;
        _clockTimerData.duree = alarm["duration"] | 0;
    }
}
// --- Identification ---
uint8_t ClockTimerMsg::getMessageTypeId() const
{
    return ((uint8_t)(LoRaMsgType::CLOCK_TIMER));
}

// --- Affichage des valeurs ---
void ClockTimerMsg::printLoRaValues(void) const
{
    printf(">-- ClockTimerMsg id = %d ---\n", getMessageTypeId());
    printLoraHeader(&(_clockTimerData.header));
    printf("--- Data ---\n");
    printf(" -- Bloc Heure Courante --\n");
    printf("  heure: %d\n", _clockTimerData.heure);
    printf("  minute: %d\n", _clockTimerData.minute);
    printf("  seconde: %d\n", _clockTimerData.seconde);
    printf(" --  Mode Alarme --\n");
    printf("  alarmMode: %d\n", _clockTimerData.alarmMode);
    printf(" -- Bloc Heure Alarme --\n");
    printf("  hAlarme: %d\n", _clockTimerData.hAlarme);
    printf("  mAlarme: %d\n", _clockTimerData.mAlarme);
    printf("  sAlarme: %d\n", _clockTimerData.sAlarme);
    printf(" --  Durée --\n");
    printf("  duree: %d\n", _clockTimerData.duree);
    printf("--- ClockTimerMsg --<\n");
}