#include <stdio.h>
#include <queue> // std::queue
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/multicore.h"
#include "hardware/irq.h"

#include <LoRa.h>
#include <LoRaLink.h>

#include <MQTTLink.h>

#include "ClockTimerMsg.h"
#include "SolarMeasureMsg.h"

#include "HubFactory.h"

const uint LED_HUB_PIN = 4; // pour pico W, cabler la led!
#if 0
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
#endif
// Instance de la factory pour conversion des messages.
HubFactory hubFactory;

static volatile bool msgReceivedFromMQTT;
// --- Le Handler (La fonction appelée lors de l'interruption declenche par core0) ---
void handler_fifo_core1()
{
    multicore_fifo_clear_irq();
    msgReceivedFromMQTT = true;
}
// Fonction qui va tourner EXCLUSIVEMENT sur le Core 1
void core1_entry()
{
    // Instance de la classe LoRa
    LoRaClass loRa;
    LoRaLink *loRaLink;
    // Buffer pour recevoir les messages
    uint8_t receivedData[128]; // Taille maximale de 128 bytes

    // On récupère le numéro d'IRQ correct pour le cœur actuel (Core 1)
    uint irq_num = SIO_FIFO_IRQ_NUM(get_core_num());
    // Configurer l'interruption FIFO sur ce Core
    multicore_fifo_clear_irq(); // Nettoyage initial
    // On attache le handler
    irq_set_exclusive_handler(irq_num, handler_fifo_core1);
    // On active l'IRQ
    msgReceivedFromMQTT = false; // On n'a rien recu.
    irq_set_enabled(irq_num, true);

    printf("Initialisation du module LoRa...\n");
    // Initialiser le module LoRa
    if (!loRa.init())
    {
        while (1)
        {
            printf("Erreur d'initialisation du module LoRa!\n");
            sleep_ms(1000);
        }
    }
    // Configurer la puissance de transmission (17 dBm)
    loRa.setTxPower(17);
    // Passer en mode réception continue
    loRa.rxContinuous();
    // Init pour methode haut niveau.
    loRaLink = new LoRaLink(&loRa, LoRaLink::NodeType::HUB);

    printf("[Core %d Interface LoRa] Lancé et prêt !\n", get_core_num());

    while (1)
    {

        // On regarde d'abord si on a une reception d'un message LoRa.
        //------------------------------------------------------------
        LoRaLink::AckStatus returnValue = loRaLink->noBlockReceive(receivedData, LoRaNodeIdType::HUB);
        if ((returnValue == LoRaLink::AckStatus::OK) ||
            (returnValue == LoRaLink::AckStatus::LOST_MESSAGE))
        {
            printf("[Core 1] Reception d'un message LoRa\n");

            // On vient de recevoir un message, envoie de l'Ack.
            LoRaLink::AckStatus status = loRaLink->sendAck((LoRaHeader *)receivedData);
            if (status == LoRaLink::AckStatus::OK)
            {
                printf("[Core 1] Message Ack envoyé! \n");
            }
            else
            {
                printf("[Core 1] Pb d'envoie du message Ack! \n");
            }

            if (multicore_fifo_wready())
            {
                // Transmet le message au core 0
                BaseMessage *msg = hubFactory.getInstanceMessage(((LoRaHeader *)receivedData)->msgType);
                if (msg != nullptr)
                {
                    // Mise a jour du message.
                    msg->updateWithLoRa((const uint8_t *)receivedData);

                    // On prévient le Core 0 par la FIFO
                    multicore_fifo_push_blocking(msg->getMessageTypeId());
                    printf("[Core 1] Message LoRa id=%d envoyé au Core 0.\n",
                           msg->getMessageTypeId());
                }
                else
                {
                    printf("[Core 1] Message LoRa id=%d inconnu dans HubFactory!\n",
                           ((LoRaHeader *)receivedData)->msgType);
                }
            }
            else
            {
                // FIFO Pleine : On abandonne le message.
                printf("[Core 1] Erreur : FIFO core1->core0 pleine, message LoRa ignoré.\n");
            }
        }

#if 0
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
                        printf(" [Core 1] heure %d minute %d \n",
                               ((ClockTimerData *)(msg->getLoRaPayload()))->heure,
                               ((ClockTimerData *)(msg->getLoRaPayload()))->minute);
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
            }
            else
            {
                mutex_exit(&mqttToLoraQueueMutex);
            }
        }
#endif
        // Petite pause de courtoisie pour laisser le bus souffler
        // (quelques microsecondes, imperceptible pour l'utilisateur)
        tight_loop_contents();
    }
}

// Dans ton main (Core 0)
int main()
{

    int commande = 0;
    int indice = 0;

    stdio_init_all();

    // Petite pause pour laisser le temps d'ouvrir le moniteur série.
    gpio_init(LED_HUB_PIN);
    gpio_set_dir(LED_HUB_PIN, GPIO_OUT);
    for (int i = 10; i > 0; i--)
    {
        printf("Demarrage dans %ds...  \r", i);

        gpio_put(LED_HUB_PIN, 1); // Allumer
        sleep_ms(500);            // Attendre 500ms

        gpio_put(LED_HUB_PIN, 0); // Eteindre
        sleep_ms(500);            // Attendre 500ms
    }

    // On lance le Core 1
    multicore_launch_core1(core1_entry);

    printf("[Core 0] Interface MQTT]\n");

    MQTTLink mqttLink;

    // 1. Déclarer la structure
    ip_addr_t broker_ip;

    // 2. Convertir la chaîne en structure ip_addr_t
    // La fonction retourne 1 si l'adresse est valide, 0 sinon
    if (!ipaddr_aton("192.168.1.16", &broker_ip))
    {
        printf("Adresse IP du broker invalide !\n");
    }

    // 3. Passer le pointeur à ta fonction
    MqttStatus status = mqttLink.mqtt_connectTimeOut(&broker_ip, 10000);
    if (status != MqttStatus::OK)
    {
        while (1)
        {
            printf("[Core 0] Probleme de connection MQTT!\n");
            sleep_ms(1000);
        }
    }
    // Init de la factory.
    hubFactory.init(&mqttLink);

    while (1)
    {

        // Reception d'un message en provenance de core 1.
        // On regarde si il y a un message du Core 1.
        if (multicore_fifo_rvalid())
        {
            uint32_t msgFifoCore = multicore_fifo_pop_blocking();
            printf("[Core 0] J'ai reçu le code : %lu\n", msgFifoCore);

            BaseMessage *msg = hubFactory.getInstanceMessage((LoRaMsgType)msgFifoCore);

            MqttStatus status = mqttLink.mqtt_publishTimeOut(msg->getMqttPublicationTopic(), msg->getMqttJson().c_str(), 10000);
            if (status == MqttStatus::OK)
            {
                printf(" [Core 0] MqttStatus::OK message MQTT envoyé: topic: %s Json:%s\n",
                       msg->getMqttPublicationTopic(),
                       msg->getMqttJson().c_str());
            }
            else if (status == MqttStatus::TIMEOUT)
            {
                printf(" [Core 0] MqttStatus::TIMEOUT message MQTT: topic: %s Json:%s\n",
                       msg->getMqttPublicationTopic(),
                       msg->getMqttJson().c_str());
            }
            else if (status == MqttStatus::ERREUR)
            {
                printf(" [Core 0] MqttStatus::ERREUR message MQTT: topic: %s Json:%s\n",
                       msg->getMqttPublicationTopic(),
                       msg->getMqttJson().c_str());
            }
        }
    }
    // Petite pause de courtoisie pour laisser le bus souffler
    // (quelques microsecondes, imperceptible pour l'utilisateur)
    tight_loop_contents();
}
