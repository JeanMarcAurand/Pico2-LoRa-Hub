#include <stdio.h>
#include <queue> // std::queue
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/multicore.h"

#include "ClockTimerMsg.h"
#include "SolarMeasureMsg.h"

typedef struct MqttMessage4Fifo
{
    char topic[64];
    char json[256];
} MqttMessage4Fifo;
MqttMessage4Fifo mqttMessage4Fifo;

typedef struct LoRaMessage4Fifo
{
    char message[128];
} LoRaMessage4Fifo;
LoRaMessage4Fifo loRaMessage4Fifo;

// Les Queues (une pour chaque sens)
std::queue<BaseMessage *> loraToMqttQueue;
auto_init_mutex(loraToMqttQueueMutex);
std::queue<BaseMessage *> mqttToLoraQueue;
auto_init_mutex(mqttToLoraQueueMutex);
enum class coreCommand
{
    CORE0_SEND_LORA_MSG_TO_CORE1,
    CORE1_SEND_MQTT_MSG_TO_CORE0
};

// Fonction qui va tourner EXCLUSIVEMENT sur le Core 1
void core1_entry()
{
    int indice = 0;

    printf("[Core 1 Interface LoRa] Lancé et prêt !\n");

    while (1)
    {

        // Reception d'un message en provenance de core 0.
        if (multicore_fifo_wready())
        {
            uint32_t msgFifoCore = multicore_fifo_pop_blocking();
            printf("[Core 1] J'ai reçu le code : %lu\n", msgFifoCore);

            mutex_enter_blocking(&mqttToLoraQueueMutex);
            if (!mqttToLoraQueue.empty())
            {
                BaseMessage *msg = mqttToLoraQueue.front();
                mqttToLoraQueue.pop();
                mutex_exit(&mqttToLoraQueueMutex);
                printf("[Core 1] J'ai reçu le message MQTT : %lu\n", msg->getMessageTypeId());

                // verifie qu'il y a correspodance entre la fifo des core et la fifo de message.
                if (msgFifoCore == msg->getMessageTypeId())
                {
                    switch (msg->getMessageTypeId())
                    {
                    case (uint8_t)LoRaMsgType::CLOCK_TIMER:
 /*                      printf(" [Core 1] heure %d minute %d \n",
                               ((ClockTimerData *)(msg->getLoRaPayload()))->heure,
                               ((ClockTimerData *)(msg->getLoRaPayload()))->minute);*/
                        // Action ! Envoie le message sur LoRa.
                        printf("[Core 1] Envoie message LoRa\n");
                        break;

                    default:
                        printf("[Core 1] Message de type inattendu = %d\n", msg->getMessageTypeId());
                        break;
                    }
                }
                else
                {
                    printf("[Core 1] Décalage entre fifo des cores id=%d et fifo messages id = %d\n",
                           msgFifoCore, msg->getMessageTypeId());
                }
                // CRUCIAL : C'est ici que le destructeur virtuel (~BaseMessage)
                // sauve ta RAM !
                delete msg;
            }
            else
            {
                mutex_exit(&mqttToLoraQueueMutex);
            }
        }

        // Simulation de la reception d'un message LoRa et envoie sur core 0.
        // On simule que l'on vient de recevoir un message Lora.
        // On simule son contenu:
        char message[128];
        SolarData *solarData = (SolarData *)message;
        solarData->header.dstNodeID = LoRaNodeIdType::HUB;
        solarData->header.msgType = LoRaMsgType::SOLAR_MEASURE;
        solarData->header.srcNodeID = LoRaNodeIdType::SOLAR_MEASURE;
        solarData->iSolar = indice + 1;
        solarData->tempRaw = indice + 2;
        solarData->vBat = indice + 3;
        indice++;
        printf("[Core 1] Reception d'un message LoRa\n");

        if (multicore_fifo_wready())
        {
            //
            SolarMeasureMsg *msg = SolarMeasureMsg::getInstance();
            msg->updateWithLoRa((const uint8_t *)message);

            // Protection de la queue
            mutex_enter_blocking(&loraToMqttQueueMutex);
            loraToMqttQueue.push(msg);
            mutex_exit(&loraToMqttQueueMutex);

            // On prévient le Core 0 par la FIFO
            multicore_fifo_push_blocking(msg->getMessageTypeId());
            printf("[Core 1] Message LoRa envoyé au Core 0\n");
        }
        else
        {
            // FIFO Pleine : On abandonne le message
            printf("[Core 1] Erreur : FIFO core1->core0 pleine, message LoRa ignoré.\n");
        }

        printf(" [Core 1] Sommeil!\n");
        sleep_ms(5000);
    }
}

// Dans ton main (Core 0)
int main()
{

    int commande = 0;
    int indice = 0;

    stdio_init_all();

    // On lance le Core 1
    multicore_launch_core1(core1_entry);

    printf("[Core 0 Interface MQTT]\n");

    while (1)
    {

        // Simulation de la reception d'un message MQTT et envoie à core 1.
        // On simule que l'on vient de recevoir un message MQTT.
        // On simule son contenu:
        char message[128];
        snprintf(message, 128, "{ \"current_time\": { \"h\": %d, \"m\": 30, \"s\": 0 }, \"alarm\": { \"mode\": 2, \"h\": 7, \"m\": 15,\"duration\": 60 }}",
                 indice % 24);
        indice++;
        printf("[Core 0] Reception d'un message MQTT\n");

        if (multicore_fifo_wready())
        {
            //
            ClockTimerMsg *msg =  ClockTimerMsg::getInstance();
            msg->updateWithMqtt((const char *)message);

            // Protection de la queue
            mutex_enter_blocking(&mqttToLoraQueueMutex);
            mqttToLoraQueue.push(msg);
            mutex_exit(&mqttToLoraQueueMutex);

            // On prévient le Core 0 par la FIFO
            multicore_fifo_push_blocking(msg->getMessageTypeId());
            printf("[Core 0] Message MQTT envoyé au Core 1\n");
        }
        else
        {
            // FIFO Pleine : On abandonne le message
            printf("Erreur : FIFO core0->core1 pleine, message MQTT ignoré.\n");
        }

        // Reception d'un message en provenance de core 1.
        // On regarde si il y a un message du Core 1.
        if (multicore_fifo_rvalid())
        {
            uint32_t msgFifoCore = multicore_fifo_pop_blocking();
            printf("[Core 0] J'ai reçu le code : %lu\n", msgFifoCore);

            mutex_enter_blocking(&loraToMqttQueueMutex);
            if (!loraToMqttQueue.empty())
            {
                BaseMessage *msg = loraToMqttQueue.front();
                loraToMqttQueue.pop();
                mutex_exit(&loraToMqttQueueMutex);
                printf("[Core 0] J'ai reçu le message LoRa : %lu\n", msg->getMessageTypeId());

                // verifie qu'il y a correspodance entre la fifo des core et la fifo de message.
                if (msgFifoCore == msg->getMessageTypeId())
                {
                    switch (msg->getMessageTypeId())
                    {
                    case (uint8_t)LoRaMsgType::SOLAR_MEASURE:
                        printf(" [Core 0] iSolar %d tempRaw %d \n",
                               ((SolarData *)(msg->getLoRaPayload()))->iSolar,
                               ((SolarData *)(msg->getLoRaPayload()))->tempRaw);
                        // Action ! (Polymorphisme : appelle le bon topic et le bon JSON)
                        // mqttClient.publish(msg->getMqttTopic(), msg->getMqttJson());
                        printf(" [Core 0] Envoie message MQTT: topic: %s Json:%s\n",
                               msg->getMqttPublicationTopic(),
                               msg->getMqttJson().c_str());
                        break;

                    default:
                        printf(" [Core 0] Message de type innattendu = %d\n", msg->getMessageTypeId());
                        break;
                    }
                }
                else
                {
                    printf(" [Core 0] Décalage entre fifo des core id=%d et fifo message id = %d\n",
                           msgFifoCore, msg->getMessageTypeId());
                }

                // CRUCIAL : C'est ici que le destructeur virtuel (~BaseMessage)
                // sauve ta RAM !
                delete msg;
            }
            else
            {
                mutex_exit(&loraToMqttQueueMutex);
            }
        }
        printf(" [Core 0] Sommeil!\n");
        sleep_ms(5000);
    }
}