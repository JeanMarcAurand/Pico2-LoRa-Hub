
#include <LoRa.h>
#include <LoRaLink.h>
#include <PersistantParams.h>
#include "powerDownAndReboot.h"

#include <stdio.h>
#include <algorithm>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "../../common/interface/testDialogLora.h"

#include "pico/util/datetime.h"
#include "hardware/rcp.h"

// Instance de la classe LoRa
LoRaClass loRa;
LoRaLink *loRaLink;

// Définir le mode: DIALOG_LORA_MODE_NODE ou DIALOG_LORA_MODE_HUB
// Compilez avec -DDIALOG_LORA_MODE_NODE pour simuler un noeud ( capteur...)
// Compilez avec -DDIALOG_LORA_MODE_HUB  pour le hub ( reception mesure et converion MQTT par exemple) ou rien par defaut

// Constantes pour les modes
#define LORA_NODE 1
#define LORA_HUB 2

#ifdef DIALOG_LORA_MODE_NODE
#define LORA_MODE LORA_NODE // Mode émetteur
#else
#define LORA_MODE LORA_HUB // Mode par défaut: récepteur
#endif

const uint LED_HUB_PIN = 4;   // pour pico W, cabler la led!
const uint LED_NODE_PIN = 25; // pour pico sans W, led intédrée.

#include "hardware/powman.h"
#include "hardware/rcp.h"
#include <cmath>

// Fonction principale pour le mode node.
//=======================================
void runForNodeMode()
{
    // Durée de sommeil.
    const int recurrence_ms = 10000;
    // Ajout ou pas de jitter sur la recurrence.
    bool whithJitter = false;

    // Buffer pour le message/
    TestDialogLoraMsg message;

    // Buffer pour l'Ack
    LoRaHeader receivedAck;

    /// init des champs fixes du Header.
    printf("Taille du message TestDialogLoraMsg=%d\n", sizeof(TestDialogLoraMsg));

    message.header.srcNodeID = LoRaNodeIdType::TEST_DIALOG_LORA;
    message.header.dstNodeID = LoRaNodeIdType::HUB;
    message.header.msgType = LoRaMsgType::TEST_DIALOG_LORA;

    // Boucle principale
    while (true)
    {
        printf("Mode Node active\n");

        // Construire le message .
        //------------------------
        message.solarCurrent = 111;
        message.batteryMv = 222;
        message.temperature = 33.3;

        // Envoyer le message.
        //--------------------
        LoRaLink::AckStatus returnValue = loRaLink->sendLoRaMessage(2000,
                                                                    (LoRaHeader *)&message,
                                                                    sizeof(TestDialogLoraMsg));
        switch (returnValue)
        {
        case LoRaLink::AckStatus::ERREUR:
            // Decale si eventuelles colisions.
            whithJitter = true;
            printf("Envoi du message: TestDialogLoraMsg! ERREUR!\n");
            break;
        case LoRaLink::AckStatus::LOST_MESSAGE:
            whithJitter = false;
            printf("Envoi du message: TestDialogLoraMsg! LOST_MESSAGE!\n");
            break;
        case LoRaLink::AckStatus::OK:
            whithJitter = false;
            printf("Envoi du message: TestDialogLoraMsg! OK!\n");
            break;
        case LoRaLink::AckStatus::TIMEOUT:
            // Decale si eventuelles colisions.
            whithJitter = true;
            printf("Envoi du message: TestDialogLoraMsg! TIMEOUT!\n");
            break;
        default:
            whithJitter = false;
            printf("Envoi du message: TestDialogLoraMsg! default???\n");
            break;
        }
        gpio_put(LED_NODE_PIN, 1); // Allumer.

        // Mise en sommeil de la radio.
        loRa.sleep();

        // On s'endort pour 10s et on reboot.
        powerDownAndReboot_ms(recurrence_ms, whithJitter, true);
    }
}

// Fonction principale pour le mode Hub.
//======================================
void runForHubMode()
{

    printf("Mode HUB active\n");

    gpio_put(LED_HUB_PIN, 0); // Eteint la led.

    // Configurer la puissance de transmission (17 dBm)
    loRa.setTxPower(17);

    // Buffer pour recevoir les messages
    uint8_t receivedData[128]; // Taille maximale de 128 bytes

    printf("En attente de messages LoRa...\n");

    // Passer en mode réception continue
    loRa.rxContinuous();

    // Boucle principale
    while (true)
    {
        // Attendre sans fin d'un message Lora.
        LoRaLink::AckStatus returnValue = loRaLink->receiveLoRaMessage(receivedData, LoRaNodeIdType::HUB);
        switch (returnValue)
        {
        case LoRaLink::AckStatus::ERREUR:
            // Decale si eventuelles colisions.
            printf("Reception returnValue = ERREUR!\n");
            break;
        case LoRaLink::AckStatus::LOST_MESSAGE:
            printf("Reception returnValue =  LOST_MESSAGE!\n");
            break;
        case LoRaLink::AckStatus::OK:
            printf("Reception returnValue =  OK!\n");
            break;
        case LoRaLink::AckStatus::TIMEOUT:
            // Decale si eventuelles colisions.
            printf("Reception returnValue =  TIMEOUT!\n");
            break;
        default:
            printf("Reception returnValue =  default???\n");
            break;
        }
        if ((returnValue == LoRaLink::AckStatus::OK) ||
            (returnValue == LoRaLink::AckStatus::LOST_MESSAGE))
        {
            // Un message a été reçu avec succés.
            switch (((LoRaHeader *)receivedData)->msgType)
            {
            case LoRaMsgType::SOLAR_MEASURE:
                printf(" Message SOLAR_MEASURE !\n ");
                break;
            case LoRaMsgType::TEST_DIALOG_LORA:
                printf(" Message TEST_DIALOG_LORA !\n ");
                printf("NoOccurence:%d batteryMv:%d solarCurrent:%d, temperature:%f\n",
                       ((TestDialogLoraMsg *)receivedData)->header.seqNo,
                       ((TestDialogLoraMsg *)receivedData)->batteryMv,
                       ((TestDialogLoraMsg *)receivedData)->solarCurrent,
                       ((TestDialogLoraMsg *)receivedData)->temperature);
                break;
            default:
                printf(" Id de message inconnu = %d!\n ", ((LoRaHeader *)receivedData)->msgType);
                break;
            }
        }
    }
}

int main()
{
    // Initialiser la carte Pico
    stdio_init_all();
    /* Test de base avant d'aller plus loin ;-) */
    // 1. Initialiser les sorties

    gpio_init(LED_NODE_PIN);
    gpio_set_dir(LED_NODE_PIN, GPIO_OUT);
    gpio_init(LED_HUB_PIN);
    gpio_set_dir(LED_HUB_PIN, GPIO_OUT);
#if 0

    int compteur = 0;

    // Boucle infinie pour tests ;-)
    while (true) {
        printf("Blink numero : %d\n", compteur++);
        
        gpio_put(LED_NODE_PIN, 1); // Allumer
        gpio_put(LED_HUB_PIN, 1); // Allumer
         sleep_ms(500);        // Attendre 500ms
        
        gpio_put(LED_NODE_PIN, 0); // Eteindre
        gpio_put(LED_HUB_PIN, 0); // Eteindre
        sleep_ms(500);        // Attendre 500ms
    }
#endif
    // Petite pause pour te laisser le temps d'ouvrir le moniteur série
    for (int i = 10; i > 0; i--)
    {
        printf("Demarrage dans %ds...  \r", i);

        gpio_put(LED_NODE_PIN, 1); // Allumer
        gpio_put(LED_HUB_PIN, 1);  // Allumer
        sleep_ms(500);             // Attendre 500ms

        gpio_put(LED_NODE_PIN, 0); // Eteindre
        gpio_put(LED_HUB_PIN, 0);  // Eteindre
        sleep_ms(500);             // Attendre 500ms
    }

    // On dit au Power Manager d'utiliser l'oscillateur basse consommation.
    powman_timer_set_1khz_tick_source_lposc();

    // Attendre un court instant que l'oscillateur soit stable.
    sleep_ms(1);

    // 1. On demerre le timer du powman.
    powman_timer_start();

    printf("Initialisation du module LoRa...\n");

    // Initialiser le module LoRa
    if (!loRa.init())
    {
        printf("Erreur d'initialisation du module LoRa!\n");
        return -1;
    }

    printf("Module LoRa initialise avec succes.\n");

    // Choisir le mode en fonction de la clé de compilation
    switch (LORA_MODE)
    {
    case LORA_NODE:
        // Init pour methode haut niveau.
        loRaLink = new LoRaLink(&loRa, LoRaLink::NodeType::NODE);
        // Simule un node.
        runForNodeMode();
        break;
    case LORA_HUB:
        // Init pour methode haut niveau.
        loRaLink = new LoRaLink(&loRa, LoRaLink::NodeType::HUB);
        // Simule un hub.
        runForHubMode();
        break;
    default:
        printf("Mode non valide.\n");
        return -1;
    }

    return 0;
}