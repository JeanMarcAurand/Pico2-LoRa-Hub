#ifndef LORA_LINK_H
#define LORA_LINK_H

#include <LoRa.h>
#include "HubPowerController.h"
#include "NodePowerController.h"

#include "../../common/interface/protocoleLora.h"

class LoRaLink
{
private:
    // Instance de la classe LoRa
    LoRaClass *_loRa; // Référence vers l'instance transmise à la construction ( unique).

    // Instance de la gestion de la puissance d'émission fonction du type de noeud.
    PowerController *_powerController;

public:
    enum class NodeType
    {
        HUB,
        NODE
    };

    // On définit les résultats possibles de l'attente
    enum class AckStatus
    {
        OK,
        TIMEOUT,
        LOST_MESSAGE,
        ERREUR
    };

    LoRaLink(LoRaClass *loRa, NodeType nodeType)
    {
        _loRa = loRa;
        if (nodeType == NodeType::HUB)
        {
            _powerController = new HubPowerController(_loRa);
        }
        else
        {
            _powerController = new NodePowerController(_loRa);
        }
    }

    AckStatus waitForReceiveLowPower(uint8_t *inMessage,
                                     LoRaNodeIdType receiveNodeIdType);
    AckStatus waitForReceiveLowPower(uint32_t timeout_ms,
                                     uint8_t *inMessage,
                                     LoRaNodeIdType srcNodeIdType,
                                     LoRaNodeIdType dstNodeIdType);
    AckStatus waitForAckLowPower(uint32_t timeout_ms,
                                 LoRaHeader *outAck,
                                 LoRaNodeIdType srcNodeIdType,
                                 LoRaNodeIdType dstNodeIdType);

    AckStatus sendBlocking(LoRaHeader *outMessage,
                           size_t size);

    AckStatus sendAck(LoRaHeader *outAck);

    AckStatus sendLoRaMessage(uint32_t ackTimeout_ms,
                              LoRaHeader *outLoRaMessage,
                              size_t size);
    AckStatus receiveLoRaMessage(uint8_t *inMessage,
                             LoRaNodeIdType dstNodeIdType);
};
#endif