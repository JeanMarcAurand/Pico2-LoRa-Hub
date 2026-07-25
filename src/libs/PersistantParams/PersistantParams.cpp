#include "PersistantParams.h"

void PersistantParams::init()
{
    if (powman_hw->scratch[idx(IndexOfPersistantsParams::MAGIC_KEY)] != MAGIC_VALUE)
    {
        // Peu probable de trouver cette valeur à la mise sous tension.
        // En revanche sera maintenu suite à un reboot.
        setNoOccurrence(0);
        setPrevSNR(-32);
        setPrevRSSI(-200);
        setTxPower(17); // Puissance max par défaut
        setNbMissingAck(0);
        powman_hw->scratch[idx(IndexOfPersistantsParams::MAGIC_KEY)] = MAGIC_VALUE;
    }
}

// Gestion du SNR (signé int32_t)
void PersistantParams::setPrevSNR(int32_t snr)
{
    powman_hw->scratch[idx(IndexOfPersistantsParams::PREV_SNR)] = static_cast<uint32_t>(snr);
}

int32_t PersistantParams::getPrevSNR()
{
    return static_cast<int32_t>(powman_hw->scratch[idx(IndexOfPersistantsParams::PREV_SNR)]);
}

// Gestion du RSSI (signé int32_t)
void PersistantParams::setPrevRSSI(int32_t rssi)
{
    powman_hw->scratch[idx(IndexOfPersistantsParams::PREV_RSSI)] = static_cast<uint32_t>(rssi);
}

int32_t PersistantParams::getPrevRSSI()
{
    return static_cast<int32_t>(powman_hw->scratch[idx(IndexOfPersistantsParams::PREV_RSSI)]);
}

// Gestion de la Puissance (uint32_t)
void PersistantParams::setTxPower(uint32_t power)
{
    powman_hw->scratch[idx(IndexOfPersistantsParams::TX_POWER)] = power;
}

uint32_t PersistantParams::getTxPower()
{
    return powman_hw->scratch[idx(IndexOfPersistantsParams::TX_POWER)];
}

// Gestion du d'ack perdu (uint32_t)
void PersistantParams::setNbMissingAck(uint32_t nbMissingAck)
{
    powman_hw->scratch[idx(IndexOfPersistantsParams::NB_MISSING_ACK)] = nbMissingAck;
}

uint32_t PersistantParams::getNbMissingAck()
{
    return powman_hw->scratch[idx(IndexOfPersistantsParams::NB_MISSING_ACK)];
}

// Gestion du noOccurrence (uint32_t)
void PersistantParams::setNoOccurrence(uint32_t noOccurrence)
{
    powman_hw->scratch[idx(IndexOfPersistantsParams::NO_OCCURENCE)] = noOccurrence;
}

uint32_t PersistantParams::getNoOccurrence()
{
    return powman_hw->scratch[idx(IndexOfPersistantsParams::NO_OCCURENCE)];
}
