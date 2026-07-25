#ifndef HUB_POWER_CONTROLLER_H
#define HUB_POWER_CONTROLLER_H
#include "PowerController.h"
#include "protocoleLora.h"
#include "LoRa.h"

#include <cstdint>
class HubPowerController : public PowerController
{
private:
    /**
     * @brief Paramètres de gestion de la liaison (Link Adaptation)
     * * DESCRIPTION DU FLUX DE DONNÉES :
     * ===============================
     * 1. [A -> B] : Le Node A envoie un message. Le Node B mesure le SNR/RSSI à la réception.
     * 2. [Stock]  : Le Node B stocke ces mesures dans 'prevSNR' et 'prevRSSI'.
     * 3. [B -> A] : Le Node B envoie un message(ACK). Il injecte ses 'prevSNR/RSSI' dans le Header LoRa.
     * 4. [A recv] : Le Node A reçoit le message, extrait les mesures faites par B.
     * 5. [Calcul] : Le Node A met à jour son 'TxPower' via l'algorithme d'asservissement.
     * 6. [A -> B] : Le prochain envoi de A se fera avec la puissance optimisée.
     * * @note : 'prevSNR' et 'prevRSSI' sont donc les miroirs de ce que l'autre côté a perçu.
     */
    struct LinkParameters
    {
        /** * @brief Métriques du dernier message REÇU (Mesurées localement).
         * Ces valeurs seront envoyées au nœud distant dans le prochain Header
         * pour qu'il puisse ajuster SA puissance d'émission.
         */
        int16_t prevSNR;  ///< SNR mesuré localement lors du dernier message reçu (-32 à +31 dB).
        int16_t prevRSSI; ///< RSSI mesuré localement lors du dernier message reçu (-150 à +10 dB).

        /** * @brief Puissance d'émission (Calculée pour le prochain ENVOI).
         * Calculée à partir du SNR/RSSI reçu dans le Header du dernier message
         * (mesures faites par le nœud distant sur notre envoi précédent).
         */
        int16_t TxPower; //< Puissance (dBm) à utiliser pour notre prochain envoi (2 à +20 dBm).

        /** @brief Compteur d'échecs pour l'asservissement de sécurité */
        int16_t nbMissingAck; ///< Compteur d'échecs (sécurité pour repasser en Full Power)

        /** @brief Numéro d'occurence du message. Est global pour le node ( pas par message ). */
        int16_t noOcuurrence; ///< Numéro d'occurence du message.
    };
    LinkParameters linksParametters[static_cast<uint8_t>(LoRaNodeIdType::NB_OF_NODEID)];
    LinkParameters *getLinkParametters(LoRaNodeIdType loRaNodeIdType)
    {
        return (&linksParametters[static_cast<uint8_t>(loRaNodeIdType)]);
    }

    // Instance de la classe LoRa
    LoRaClass *_loRa; // Référence vers l'instance transmise à la construction ( unique).

    LoRaNodeIdType _activeNode; // Le nœud en cours de traitement

public:
    HubPowerController(LoRaClass *loRa)
    {
        _loRa = loRa;

        // Init des paramètres.
        for (int indiceParametre = 0; indiceParametre < (int)(LoRaNodeIdType::NB_OF_NODEID); indiceParametre++)
        {
            LinkParameters *linkParametters = getLinkParametters(static_cast<LoRaNodeIdType>(indiceParametre));
            linkParametters->nbMissingAck = 0;
            linkParametters->prevRSSI = -200;
            linkParametters->prevSNR = -32;
            linkParametters->TxPower = 17;
        }
    }

    void setActiveNode(LoRaNodeIdType id) { _activeNode = id; }

    void updateLinkPower(int32_t snr, int32_t rssi) override
    {
        getLinkParametters(_activeNode)->TxPower =
            _loRa->computeNextTxPower(snr, rssi, getLinkParametters(_activeNode)->TxPower);
    }

    int32_t getLinkPower() override
    {
        return getLinkParametters(_activeNode)->TxPower;
    }

    // Gestion du SNR (signé int32_t)
    void setPrevSNR(int32_t snr)
    {
        getLinkParametters(_activeNode)->prevSNR = snr;
    }
    int32_t getPrevSNR()
    {
        return (getLinkParametters(_activeNode)->prevSNR);
    }

    // Gestion du RSSI (signé int32_t)
    void setPrevRSSI(int32_t rssi)
    {
        getLinkParametters(_activeNode)->prevRSSI=rssi;
    }
    int32_t getPrevRSSI()
    {
        return (getLinkParametters(_activeNode)->prevRSSI);
    }

    // Gestion de la Puissance (uint32_t)
    void setTxPower(uint32_t power)
    {
        getLinkParametters(_activeNode)->TxPower = power;
    }
    uint32_t getTxPower()
    {
        return (getLinkParametters(_activeNode)->TxPower);
    }
    // Gestion de la perte des Ack.
    void setNbMissingAck(uint32_t ackNumber)
    {
        getLinkParametters(_activeNode)->nbMissingAck = ackNumber;
    }
    virtual uint32_t getNbMissingAck()
    {
        return (getLinkParametters(_activeNode)->nbMissingAck);
    }

    // Gestion du noOccurrence (uint32_t)
    void setNoOccurrence(uint32_t noOccurrence)
    {
        getLinkParametters(_activeNode)->noOcuurrence = noOccurrence;
    }
    uint32_t getNoOccurrence()
    {
        return (getLinkParametters(_activeNode)->noOcuurrence);
    }
};
#endif