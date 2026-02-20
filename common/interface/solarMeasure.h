#ifndef SOLAR_MEASURE_H
#define SOLAR_MEASURE_H

#include "protocoleLora.h"

/**
 * Message spécifique au capteur de rayonnement solaire
 */
struct __attribute__((packed)) SolarMeasureMsg {
    LoRaHeader header;      // Toujours commencer par le header (4 octets)
    
    // Données spécifiques (exemples)
    uint16_t solarCurrent;  // 2 octets pour tester
    uint16_t batteryMv;     // Tension batterie en millivolts
    int16_t  temperature;   // Température locale (coeff 0.1°C)
};

#endif