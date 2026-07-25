
/**
 * @file protocol_lora.h
 * @brief Définition du protocole de communication LoRa et du Header personnalisé.
 */

#ifndef PROTOCOL_LORA_H
#define PROTOCOL_LORA_H

#include <cstdint>

/**
 * @enum LoRaNodeIdType
 * @brief Identifiants uniques des nœuds du réseau (0 à 63).
 */
enum class LoRaNodeIdType : uint8_t
{
    HUB = 0,          ///< Concentrateur central (Maître)
    SOLAR_MEASURE,    ///< Nœud de mesure solaire
    CLOCK_TIMER,      ///< Données d'heure et d'alame.
    TEST_DIALOG_LORA, ///< Nœud de test et diagnostic
    BROADCAST_MSG,    ///< Message emis en broadcast.
    NB_OF_NODEID      ///< Compteur pour l'itération et la taille des tableaux
};

/**
 * @enum LoRaMsgType
 * @brief Types de messages circulant sur le réseau LoRa.
 * * /!\ RÈGLES D'OR DE SÉCURITÉ ARCHITECTURALE (COMPATIBILITÉ ASCENDANTE) /!\
 * * Ce protocole lie le Hub et des Nœuds autonomes (qui peuvent ne pas être mis à jour 
 * en même temps). Pour éviter de décaler les IDs et de casser les équipements en place :
 * * 1. REGLE DU "APPEND ONLY" : Tout nouveau message doit TOUJOURS être ajouté à la fin,
 * juste AVANT 'NB_OF_MSG'.
 * 2. VALEURS EXPLICITES : Chaque ID doit avoir sa valeur numérique écrite en dur (= X).
 * 3. JAMAIS D'INSERTION : N'insérez jamais un nouvel ID au milieu de la liste.
 * 4. JAMAIS DE RÉORGANISATION : Ne triez jamais cette liste par ordre alphabétique ou "esthétique".
 * 5. JAMAIS DE SUPPRESSION : Si un message devient obsolète, ne supprimez pas sa ligne. 
 * Renommez-le (ex: OBSOLETE_AncienMsg = 3) pour réserver sa valeur numérique pour l'éternité.
 */
enum class LoRaMsgType : uint8_t
{
    ACK              = 0,   ///< Accusé de réception (contient les métriques pour l'asservissement)
    SOLAR_MEASURE    = 1,   ///< Données de télémétrie solaire
    CLOCK_TIMER      = 2,   ///< Données d'alarme
    BOOT_REQUEST     = 3,   ///< Message de demande de boot (pour renvoi de tous les messages requis)
    CURRENT_TIME     = 4,   ///< Heure courante
    TEST_DIALOG_LORA = 5,   ///< Message de test
    
    // <-- LES NOUVEAUX MESSAGES VONT TOUJOURS ICI (ex: NOUVEAU_MSG = 6,)

    NB_OF_MSG        = 6    ///< INDEX DE SÉCURITÉ : Doit correspondre à la valeur du dernier message + 1
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

    uint32_t reserved : 1; ///< 1 bit de réserve pour alignement sur 5 octets
};

#endif // PROTOCOL_LORA_H