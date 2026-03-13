#ifndef PERSISTANT_PARAMS_H
#define PERSISTANT_PARAMS_H
#include <stdint.h>
#include "hardware/powman.h"

class PersistantParams
{
private:

    enum class IndexOfPersistantsParams : uint32_t
    {
        NO_OCCURENCE = 0,
        PREV_SNR = 1,
        PREV_RSSI = 2,
        TX_POWER = 3,
        NB_MISSING_ACK = 4,
        MAGIC_KEY = 5
    };

    static const uint32_t MAGIC_VALUE = 0xDEADBEEF;

    // Helper interne pour le cast de l'index
    static inline uint32_t idx(IndexOfPersistantsParams i)
    {
        return static_cast<uint32_t>(i);
    }

public:
    static void init();

    // Gestion du SNR (signé int32_t)
    static void setPrevSNR(int32_t snr);
    static int32_t getPrevSNR();

    // Gestion du RSSI (signé int32_t)
    static void setPrevRSSI(int32_t rssi);
    static int32_t getPrevRSSI();

    // Gestion de la Puissance (uint32_t)
    static void setTxPower(uint32_t power);
    static uint32_t getTxPower();

    // Gestion du d'ack perdu (uint32_t)
    static void setNbMissingAck(uint32_t nbMissingAck);
    static uint32_t getNbMissingAck();

    // Gestion du noOccurrence (uint32_t)
    static void setNoOccurrence(uint32_t noOccurrence);
    static uint32_t getNoOccurrence();
};
#endif