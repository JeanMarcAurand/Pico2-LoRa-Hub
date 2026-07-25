#ifndef BASE_MESSAGE_H
#define BASE_MESSAGE_H

#include <cstdint>
#include <string>
#include "protocoleLora.h"

class BaseMessage
{
public:
  virtual ~BaseMessage() {}

  // --- Interface pour MQTT ---
  // Cas 1 : Le Hub doit s'ABONNER à ce topic.
  // Renvoie nullptr si le Hub n'a pas besoin d'écouter pour ce message
  virtual const char *getMqttSubscriptionTopic() const = 0;
  // Cas 2 : Le Hub doit PUBLIER sur ce topic.
  // Renvoie nullptr si ce message n'est jamais envoyé en MQTT.
  virtual const char *getMqttPublicationTopic() const = 0;
  virtual const std::string getMqttJson() const = 0;
  virtual void updateWithMqtt(const char *json) = 0;

  // --- Interface pour LoRa ---
  virtual uint8_t *getLoRaPayload() const = 0;
  virtual void updateWithLoRa(const uint8_t *payload) = 0;

  // --- Identification ---
  virtual uint8_t getMessageTypeId() const = 0;

  // --- Affichage des valeurs ---
  virtual void printLoRaValues(void) const = 0;
  void printLoraHeader(const LoRaHeader *loraHeader) const
  {
    printf(" -- LoRaHeader --\n");
    printf(" srcNodeID: %d\n", loraHeader->srcNodeID);
    printf(" dstNodeID: %d\n", loraHeader->dstNodeID);
    printf(" msgType:   %d\n", loraHeader->msgType);
    printf(" seqNo:     %d\n", loraHeader->seqNo);
    printf(" prevSNR:   %d\n", loraHeader->prevSNR);
    printf(" prevRSSI:  %d\n", loraHeader->prevRSSI);
  }
  void printMQTTValues(void) const
  {
    if( getMqttPublicationTopic() != nullptr )
    {
    printf(" --- Publication topic: %s ---\n", getMqttPublicationTopic());
    }
        if( getMqttSubscriptionTopic() != nullptr )
    {
    printf(" --- Subscription topic: %s ---\n", getMqttSubscriptionTopic());
    }
    printf(" --- Json: %s ---\n", getMqttJson());
  }
};
#endif