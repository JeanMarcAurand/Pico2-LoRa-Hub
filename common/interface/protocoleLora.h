
#ifndef PROTOCOL_LORA_H
#define PROTOCOL_LORA_H

#include <cstdint>

#pragma pack(push, 1)  // On force l'alignement strict

/**
 * @brief Header de 3 octets (24 bits) optimisé pour LoRa
 * nodeID  : 6 bits (0-63)
 * msgType : 6 bits (0-63)
 * seqNo   : 6 bits (0-63)
 * length  : 6 bits (0-63 octets de payload)
 */
struct LoraHeader {
    uint32_t nodeId  : 6;
    uint32_t msgType : 6;
    uint32_t seqNo   : 6;
    uint32_t length  : 6;
    // Note : On utilise uint32_t car 24 bits ne rentrent pas dans un uint16_t, 
    // mais le compilateur n'utilisera bien que 3 octets en mémoire.
};

#pragma pack(pop)

#endif // PROTOCOL_LORA_H