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
  virtual const std::string getMqttTopic() const = 0;
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
    printf(" --- ClockTimerMsg topic: %s ---\n", getMqttTopic());
    printf(" --- Json: %s ---\n", getMqttJson());
  }
};
#endif