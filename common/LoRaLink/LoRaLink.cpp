
#include <stdio.h>
#include <algorithm>
#include "pico/stdlib.h"
#include <cmath>
#include <LoRaLink.h>

#include "pico/time.h"
#include "pico/util/datetime.h"
#include "hardware/rcp.h"

#include "pico/rand.h"

// Variable globale ou statique pour savoir si l'alarme a sonné
static volatile bool timeout_expired = false;

// Le "Callback" : ce qui se passe quand l'alarme sonne
int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    timeout_expired = true;
    return 0; // 0 signifie "ne pas répéter l'alarme"
}
LoRaLink::AckStatus LoRaLink::waitForReceiveLowPower(uint8_t *inMessage,
                                                     LoRaNodeIdType receiveNodeIdType)
{
    AckStatus returnValue = AckStatus::TIMEOUT;
    // Les it sont déjà initialisé dans LoRa pour la réception.
    // On attend sans fin, si le timer n'a pas été initialisé avant ;-)
    do
    {
        // Dodo et economie d'énergie!
        __asm volatile("wfi");

        // Au réveil, on regarde si c'est le LoRa.
        int res = _loRa->lora_event(inMessage);
        if (res > 0)
        {
            LoRaHeader *loRaHeader = (LoRaHeader *)inMessage;

            // C'est un message LoRa correct.
            if (loRaHeader->dstNodeID == receiveNodeIdType)
            {
                // On est bon,c'est bien un message qui nous est destiné.
                printf("Message recu de %d occurence %d! Carractéristique reception RSSI: %d dBm SNR: %fdB\n",
                       loRaHeader->srcNodeID, loRaHeader->seqNo,
                       _loRa->packetRssi(), _loRa->packetSnr());

                // Met a jour les infos de puissance recu et calcul de la prochaine puissance d'émission.
                _powerController->setActiveNode(loRaHeader->srcNodeID);
                _powerController->setPrevSNR(std::clamp((int)_loRa->packetSnr(), -32, 31));
                _powerController->setPrevRSSI(std::clamp((int)_loRa->packetRssi(), -150, 10));
                _powerController->setTxPower(_loRa->computeNextTxPower(
                    _powerController->getPrevSNR(),
                    _powerController->getPrevRSSI(),
                    _powerController->getTxPower()));

                // Verification du no d'occurence.
                int noOcc = loRaHeader->seqNo;
                if (_powerController->getNoOccurrence() == noOcc)
                {
                    // C'est tout bon, on n'a pas perdu de messages.
                    returnValue = AckStatus::OK;
                }
                else
                {
                    // Des messages ont été perdus.
                    returnValue = AckStatus::LOST_MESSAGE;
                }
                // remet à jour le prochain no d'occurence.
                _powerController->setNoOccurrence(noOcc + 1);
            }
        }
        else if (res == -2)
        {
            // Erreur de CRC.
            printf("Message recu avec erreur de CRC\n");
        }
        else if (res == -3)
        {
            // Message trop grand.
            printf("Message trop grand pour le buffer\n");
        }

        //        printf("Sortie du wfi! \n");
    } while ((returnValue != AckStatus::OK) &&
             (returnValue != AckStatus::LOST_MESSAGE) &&
             (timeout_expired != true));

    return returnValue;
}

LoRaLink::AckStatus LoRaLink::waitForReceiveLowPower(uint32_t timeout_ms,
                                                     uint8_t *inMessage,
                                                     LoRaNodeIdType srcNodeIdType,
                                                     LoRaNodeIdType dstNodeIdType)
{

    AckStatus returnValue = AckStatus::ERREUR;
    alarm_id_t alarm_id;
    timeout_expired = false;

    // Initialisation de l'alarme.
    // On demande au timer de nous appeler dans timeout_ms.
    if (timeout_ms > 0)
    {
        alarm_id = add_alarm_in_ms(timeout_ms, alarm_callback, NULL, false);
    }
    // Les it sont déjà initialisé dans LoRa pour la réception.

    do
    {
        // Comme on a initialiser le timer l'attente n'est plus infinie, on peut sortir en timeout.
        returnValue = waitForReceiveLowPower(inMessage, srcNodeIdType);

        if ((returnValue == AckStatus::OK) ||
            (returnValue == AckStatus::LOST_MESSAGE))
        {
            LoRaHeader *loRaHeader = (LoRaHeader *)inMessage;
            // C'est un message LoRa correct.
            if ((loRaHeader->dstNodeID == dstNodeIdType) &&
                ((loRaHeader->srcNodeID == srcNodeIdType)))
            {
                // On est bon,c'est bien un message qui nous est destiné.
                returnValue = AckStatus::OK;
                // On annule l'alarme si on a reçu un message !
                cancel_alarm(alarm_id);
            }
        }
        // On s'est réveillé sans avoir rien recu, c'est un time out.
        // Verifie si c'est le timer.
        else if (timeout_expired == true)
        {
            // Révéillé par le time out.
            printf("Timeout !\n");
            returnValue = AckStatus::TIMEOUT;
        }
        else
        {
            // Curieux?
            printf("Rien recu mais pas Timeout???? !\n");
            returnValue = AckStatus::ERREUR;
        }

    } while ((returnValue != AckStatus::OK) &&
             (returnValue != AckStatus::LOST_MESSAGE) &&
             (returnValue != AckStatus::TIMEOUT));

    return returnValue;
}

LoRaLink::AckStatus LoRaLink::waitForAckLowPower(uint32_t timeout_ms,
                                                 LoRaHeader *inAck,
                                                 LoRaNodeIdType srcNodeIdType,
                                                 LoRaNodeIdType dstNodeIdType)
{
    AckStatus returnValue = AckStatus::ERREUR;
    alarm_id_t alarm_id;
    timeout_expired = false;

    // Initialisation de l'alarme.
    // On demande au timer de nous appeler dans timeout_ms.
    if (timeout_ms > 0)
    {
        alarm_id = add_alarm_in_ms(timeout_ms, alarm_callback, NULL, false);
    }
    // Les it sont déjà initialisé dans LoRa pour la réception.

    do
    {
        // Comme on a initialiser le timer l'attente n'est plus infinie, on peut sortir en timeout.
        returnValue = waitForReceiveLowPower((uint8_t *)inAck, dstNodeIdType);

        if ((returnValue == AckStatus::OK) ||
            (returnValue == AckStatus::LOST_MESSAGE))
        {
            // C'est un message LoRa correct.
            if ((inAck->msgType == LoRaMsgType::ACK) &&
                (inAck->dstNodeID == dstNodeIdType) &&
                ((inAck->srcNodeID == srcNodeIdType)))
            {
                // On est bon,c'est bien un ack qui nous est destiné.
                // On annule l'alarme si on a reçu un message !
                cancel_alarm(alarm_id);
            }
            else
            {
                // C'est pas pour nous, on se remet en attente.
                returnValue = AckStatus::ERREUR;
            }
        }
        // On s'est réveillé sans avoir rien recu, c'est un time out.
        // Verifie si c'est le timer.
        else if (timeout_expired == true)
        {
            // Révéillé par le time out.
            _powerController->setActiveNode(srcNodeIdType);
            uint32_t nombreAckPerdu = _powerController->getNbMissingAck() + 1;
            _powerController->setNbMissingAck(nombreAckPerdu);
            if (nombreAckPerdu == 1)
            {
                // On ne modifie pas la puissance.
            }
            else if (nombreAckPerdu == 2)
            {
                // On ne remonte la puissance que de 6dB.
                _powerController->setTxPower(std::clamp((int)(_powerController->getTxPower() + 6), 2, 17));
            }
            else
            {
                // On a perdu la liaison, repart en puissance max si la cause est que le Hub n'a pas reçu le message.
                _powerController->setTxPower(17);
            }
            returnValue = AckStatus::TIMEOUT;
        }
        else
        {
            // Curieux?
            printf("Rien recu mais pas Timeout???? !\n");
            returnValue = AckStatus::ERREUR;
        }
    } while ((returnValue != AckStatus::OK) &&
             (returnValue != AckStatus::LOST_MESSAGE) &&
             (returnValue != AckStatus::TIMEOUT));

    return returnValue;
}

LoRaLink::AckStatus LoRaLink::sendBlocking(LoRaHeader *outMessage,
                                           size_t size)
{
    AckStatus returnValue = AckStatus::ERREUR;

    // Configurer la puissance de transmission.
    _powerController->setActiveNode(outMessage->dstNodeID);
    _loRa->setTxPower(_powerController->getLinkPower());

    // Met à jour les dernier niveaux d'emission recus.
    outMessage->prevSNR = _powerController->getPrevSNR();
    outMessage->prevRSSI = _powerController->getPrevRSSI();

    // Met a jour le no d'occurence.
    uint32_t noOcuurrence = _powerController->getNoOccurrence();
    outMessage->seqNo = noOcuurrence;
    _powerController->setNoOccurrence(noOcuurrence + 1);

    // Met à jour le champ reserve.
    outMessage->reserved = 0;

    // Envoie du message.
    int result = _loRa->sendPacketBlocking((uint8_t *)outMessage, size);
    if (result > 0)
    {
        printf("Message  envoye avec succes. Taille: %d / %d bytes\n",
               result, size);
        returnValue = AckStatus::OK;
    }
    else
    {
        printf("Erreur lors de l'envoi du message. Code d'erreur: %d\n",
               result);
        returnValue = AckStatus::ERREUR;
    }
    // Repasser en mode réception continue.
    _loRa->rxContinuous();

    return returnValue;
}
LoRaLink::AckStatus LoRaLink::sendAck(LoRaHeader *outAck)
{
    return sendBlocking(outAck, sizeof(LoRaHeader));
}

LoRaLink::AckStatus LoRaLink::sendLoRaMessage(uint32_t ackTimeout_ms,
                                              LoRaHeader *outLoRaMessage,
                                              size_t size)
{
    AckStatus returnValue = AckStatus::ERREUR;
    // Emission du message.
    returnValue = sendBlocking(outLoRaMessage, size);
    if (returnValue == AckStatus::OK)
    {
        // L'envoie s'est bien passé, attend l'ack.
        // Buffer pour l'Ack
        LoRaHeader receivedAck;
        returnValue = waitForAckLowPower(ackTimeout_ms,
                                         &receivedAck,
                                         outLoRaMessage->dstNodeID,
                                         outLoRaMessage->srcNodeID);
    }
    return (returnValue);
}

LoRaLink::AckStatus LoRaLink::receiveLoRaMessage(uint8_t *inMessage,
                                             LoRaNodeIdType dstNodeIdType)
{
    AckStatus returnValue = AckStatus::ERREUR;

    // Attend sans time out un message.
    returnValue = waitForReceiveLowPower(inMessage, dstNodeIdType);
    if ((returnValue == AckStatus::OK) ||
        (returnValue == AckStatus::LOST_MESSAGE))
    {
        // On vient de recevoir un message, envoie de l'Ack.
        LoRaHeader ackMsg;
        LoRaNodeIdType srcNodeIdType = ((LoRaHeader *)inMessage)->srcNodeID;
        ackMsg.dstNodeID = srcNodeIdType;
        ackMsg.srcNodeID = dstNodeIdType;
        ackMsg.seqNo = ((LoRaHeader *)inMessage)->seqNo;
        ackMsg.msgType = LoRaMsgType::ACK;

        ackMsg.prevSNR = _loRa->packetSnr();
        ackMsg.prevRSSI = _loRa->packetRssi();

        LoRaLink::AckStatus status = sendAck(&ackMsg);
        if (status == LoRaLink::AckStatus::OK)
        {
            printf("Message Ack envoyé! \n");
        }
        else
        {
            printf("Pb d'envoie du message Ack! \n");
        }
    }
    return (returnValue);
}