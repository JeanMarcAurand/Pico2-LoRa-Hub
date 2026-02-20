
#ifndef PROTOCOL_LORA_H
#define PROTOCOL_LORA_H

#include <stdint.h>
enum class LoRaNodeIdType : uint8_t
{
    HUB = 0,
    SOLAR_MEASURE = 1,

    TEST_DIALOG_LORA = 63
    // On peut aller jusqu'à 63
};

enum class LoRaMsgType : uint8_t
{
    ACK = 0,
    SOLAR_MEASURE = 1,

    TEST_DIALOG_LORA = 63
    // On peut aller jusqu'à 63
};

/**
 * @brief Header LoRa compact de 4 octets (32 bits)
 * L'attribut 'packed' garantit que la taille est exactement de 4 octets.
 */
struct __attribute__((packed)) LoRaHeader
{
    LoRaNodeIdType srcNodeID : 6; // 0-63
    LoRaNodeIdType dstNodeID : 6; // 0-63
    LoRaMsgType msgType : 6;      // Type de message
    uint32_t seqNo : 6;           // Numéro de séquence
    int32_t prevSNR : 6;          // SNR signé (-32 à +31 dB)
    uint32_t reserved : 2;        // 2 bits de réserve (0-3)
};


#endif // PROTOCOL_LORA_H