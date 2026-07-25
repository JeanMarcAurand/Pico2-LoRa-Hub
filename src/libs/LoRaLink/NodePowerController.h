#ifndef NODE_POWER_CONTROLLER_H
#define NODE_POWER_CONTROLLER_H
#include "PowerController.h"
#include "PersistantParams.h"
#include "LoRa.h"

class NodePowerController : public PowerController
{
private:
    PersistantParams persistantParams;
    // Instance de la classe LoRa
    LoRaClass *_loRa; // Référence vers l'instance transmise à la construction ( unique).

public:
    NodePowerController(LoRaClass *loRa)
    {
        _loRa = loRa;
        persistantParams.init();
    }
    void setActiveNode(LoRaNodeIdType id) {}

    void updateLinkPower(int32_t snr, int32_t rssi) override
    {

        persistantParams.setTxPower(_loRa->computeNextTxPower(snr, rssi, persistantParams.getTxPower()));
    }

    int32_t getLinkPower() override
    {
        return persistantParams.getTxPower();
    }
    // Gestion du SNR (signé int32_t)
    void setPrevSNR(int32_t snr)
    {
        persistantParams.setPrevSNR(snr);
    }
    int32_t getPrevSNR()
    {
        return (persistantParams.getPrevSNR());
    }

    // Gestion du RSSI (signé int32_t)
    void setPrevRSSI(int32_t rssi)
    {
        persistantParams.setPrevRSSI(rssi);
    }
    int32_t getPrevRSSI()
    {
        return (persistantParams.getPrevRSSI());
    }

    // Gestion de la Puissance (uint32_t)
    void setTxPower(uint32_t power)
    {
        persistantParams.setTxPower(power);
    }
    uint32_t getTxPower()
    {
        return (persistantParams.getTxPower());
    }

    // Gestion de la perte des Ack.
    void setNbMissingAck(uint32_t ackNumber)
    {
        persistantParams.setNbMissingAck(ackNumber);
    }
    uint32_t getNbMissingAck()
    {
        return (persistantParams.getNbMissingAck());
    }

    // Gestion du noOccurrence (uint32_t)
    void setNoOccurrence(uint32_t noOccurrence)
    {
        persistantParams.setNoOccurrence(noOccurrence);
    }
    uint32_t getNoOccurrence()
    {
        return (persistantParams.getNoOccurrence());
    }
};
#endif