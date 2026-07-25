#ifndef HUB_FACTORY_H
#define HUB_FACTORY_H

#include <cstring>
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"

#include "BaseMessage.h"
#include "protocoleLora.h"
#include "IMqttDispatcher.h"
#include "MQTTLink.h"

class HubFactory : public IMqttDispatcher
{
private:
    // Le tableau de pointeurs vers la classe mère
    static BaseMessage *registry[static_cast<uint8_t>(LoRaMsgType::NB_OF_MSG)];

    void registerMessage(BaseMessage *baseMessage, MQTTLink *mqttLink)
    {
        // Enregistre le message.
        registry[static_cast<uint8_t>(baseMessage->getMessageTypeId())] = baseMessage;

        // S'abonne au topic MQQT s'il ya lieu.
        if (baseMessage->getMqttSubscriptionTopic() != nullptr)
        {
            mqttLink->mqtt_subscribeTimeOut(baseMessage->getMqttSubscriptionTopic(),
                                            500);
        }
    }

    static BaseMessage *memBaseMessage;

    MQTTLink *_MQTTLink;
#define MQTT_MAX_PAYLOAD_SIZE 1024
    static char mqttStaticBuffer[MQTT_MAX_PAYLOAD_SIZE];
    static uint16_t mqttBufferIndex;

public:
    HubFactory();

    void init(MQTTLink *mqttLink);

    BaseMessage *getInstanceMessage(LoRaMsgType id)
    {
        uint8_t ident = static_cast<uint8_t>(id);
        if ((ident < static_cast<uint8_t>(LoRaMsgType::NB_OF_MSG)) &&
            (ident >= 0))
        {
            return (registry[ident]);
        }
        else
        {
            printf("ERREUR : ID de message %d hors limites !\n", ident);
            return (nullptr);
        }
    }

    void dispatchPublish(const char *topic, uint32_t tot_len) override
    {
        mqttBufferIndex = 0;
        // Determine à quel message le topic correspond.
        memBaseMessage = nullptr;
        bool topicTrouve = false;
        for (int i = 0;
             (i < (int)LoRaMsgType::NB_OF_MSG) && (topicTrouve == false);
             i++)
        {
            BaseMessage *baseMessage = getInstanceMessage((LoRaMsgType)i);
            if (baseMessage != nullptr)
            {
                if (baseMessage->getMqttPublicationTopic() != nullptr)
                {
                    if (strcmp(topic, baseMessage->getMqttPublicationTopic()) == 0)
                    {
                        // On a trouver le bon message, s'en rapelle.
                        memBaseMessage = baseMessage;
                        // Se positionne au début pour stocker le Json.
                        mqttBufferIndex = 0;
                        // On arrete de parcourir les ID.
                        topicTrouve = true;
                    }
                }
            }
        }
    }

    void dispatchData(const uint8_t *data, uint16_t len, uint8_t flags) override
    {
        // Verifie qu'il y a bien eu un topic donc un message que l'on connait.
        if (memBaseMessage == nullptr)
            return;

        // Sécurité anti-débordement (Buffer Overflow) secondaire
        if (mqttBufferIndex + len < MQTT_MAX_PAYLOAD_SIZE)
        {
            // Copie directe dans notre tableau fixe
            memcpy(&mqttStaticBuffer[mqttBufferIndex], data, len);
            mqttBufferIndex += len;
        }
        else
        {
            printf("Erreur : Débordement de buffer évité en cours de réception !\n");
            memBaseMessage = nullptr;
            return;
        }

        // C'est le dernier morceau ?
        if (flags & MQTT_DATA_FLAG_LAST)
        {
            mqttStaticBuffer[mqttBufferIndex] = '\0'; // On ferme proprement la chaîne C (\0)

            // Met a jour le message et previent l'autre core.
            memBaseMessage->updateWithMqtt(mqttStaticBuffer);
            multicore_fifo_push_blocking(memBaseMessage->getMessageTypeId());
        }
    }
};

#endif