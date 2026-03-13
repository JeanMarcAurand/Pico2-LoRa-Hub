
/**
 * @file protocol_lora.h
 * @brief Définition du protocole de communication LoRa et du Header personnalisé.
 */

#ifndef PROTOCOL_LORA_H
#define PROTOCOL_LORA_H

#include <stdint.h>

/**
 * @enum LoRaNodeIdType
 * @brief Identifiants uniques des nœuds du réseau (0 à 63).
 */
enum class LoRaNodeIdType : uint8_t
{
    HUB = 0,            ///< Concentrateur central (Maître)
    SOLAR_MEASURE,      ///< Nœud de mesure solaire
    TEST_DIALOG_LORA,   ///< Nœud de test et diagnostic
    NB_OF_NODEID        ///< Compteur pour l'itération et la taille des tableaux
};

/**
 * @enum LoRaMsgType
 * @brief Types de messages circulant sur le réseau.
 */
enum class LoRaMsgType : uint8_t
{
    ACK = 0,            ///< Accusé de réception (contient les métriques pour l'asservissement)
    SOLAR_MEASURE,      ///< Données de télémétrie solaire
    TEST_DIALOG_LORA,   ///< Message de test
    NB_OF_MSG           ///< Nombre total de types de messages définis
};

/**
 * @struct LoRaHeader
 * @brief Header LoRa compact de 5 octets (40 bits).
 * * L'attribut 'packed' garantit l'absence de padding entre les membres.
 * Ce header est conçu pour optimiser le temps d'occupation du canal (Time on Air).
 */
struct __attribute__((packed)) LoRaHeader
{
    LoRaNodeIdType srcNodeID : 6; ///< ID du nœud émetteur (6 bits : 0-63)
    LoRaNodeIdType dstNodeID : 6; ///< ID du nœud destinataire (6 bits : 0-63)
    LoRaMsgType msgType : 6;      ///< Type de message (6 bits : 0-63)
    uint32_t seqNo : 6;           ///< Numéro de séquence pour détecter les pertes (6 bits : 0-63)
    
    /** * @brief SNR de l'échange précédent mesuré par l'émetteur.
     * Valeur signée sur 6 bits permettant de coder de -32 à +31 dB.
     */
    int32_t prevSNR : 6;          
    
    /** * @brief RSSI de l'échange précédent mesuré par l'émetteur.
     * Valeur signée sur 8 bits pour coder -150 à +10 dB.
     */
    int32_t prevRSSI : 9;         
    
uint32_t reserved : 1;        ///< 1 bit de réserve pour alignement sur 5 octets
};

#endif // PROTOCOL_LORA_H