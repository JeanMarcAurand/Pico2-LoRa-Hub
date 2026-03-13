#ifndef POWER_CONTROLLER_H
#define POWER_CONTROLLER_H
#include <cstdint>
#include "protocoleLora.h"

class PowerController
{
public:
    virtual void setActiveNode(LoRaNodeIdType id) = 0;

    virtual void updateLinkPower(int32_t snr, int32_t rssi) = 0;
    virtual int32_t getLinkPower() = 0;

    // Gestion du SNR (signé int32_t)
    virtual void setPrevSNR(int32_t snr) = 0;
    virtual int32_t getPrevSNR() = 0;

    // Gestion du RSSI (signé int32_t)
    virtual void setPrevRSSI(int32_t rssi) = 0;
    virtual int32_t getPrevRSSI() = 0;

    // Gestion de la Puissance (uint32_t)
    virtual void setTxPower(uint32_t power) = 0;
    virtual uint32_t getTxPower() = 0;

    // Gestion de la perte des Ack.
    virtual void setNbMissingAck(uint32_t ackNumber) = 0;
    virtual uint32_t getNbMissingAck() = 0;

    // Gestion du noOccurrence (uint32_t)
    virtual void setNoOccurrence(uint32_t noOccurrence) = 0;
    virtual uint32_t getNoOccurrence() = 0;
};
#endif