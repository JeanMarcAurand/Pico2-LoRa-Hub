#ifndef TEST_DIALOG_LORA_H
#define TEST_DIALOG_LORA_H

#include "protocoleLora.h"
#include <stdint.h>

/**
 * Message spécifique au capteur de rayonnement solaire
 */
struct __attribute__((packed)) TestDialogLoraMsg {
    LoRaHeader header;      // Toujours commencer par le header (4 octets)
    
    // Données spécifiques (exemples)
    int16_t solarCurrent;  // 2 octets signé pour tester
    uint16_t batteryMv;    // 2 octets non signé pour tester
    float  temperature;    // 4 octets  pour tester
};

#endif