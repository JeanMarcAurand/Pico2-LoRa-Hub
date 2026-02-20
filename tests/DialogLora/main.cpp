// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <LoRa.h>
#include <stdio.h>
#include <algorithm>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "../../common/interface/testDialogLora.h"

#include "hardware/powman.h"
#include "pico/util/datetime.h"
#include "hardware/rcp.h"

// Instance de la classe LoRa
LoRaClass LoRa;

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

const uint LED_HUB_PIN = 4;
const uint LED_NODE_PIN = 25;

// Variable globale ou statique pour savoir si l'alarme a sonné
static volatile bool timeout_expired = false;

// Le "Callback" : ce qui se passe quand l'alarme sonne
int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    timeout_expired = true;
    return 0; // 0 signifie "ne pas répéter l'alarme"
}

#include "hardware/powman.h"
#include "hardware/rcp.h"
#include "hardware/watchdog.h"

void powerDownAndReboot(uint32_t delay_s, bool debug)
{
    // Mise en sommeil de la radio.
    LoRa.sleep();

    // Toutes les GPIO à l'état d'usine (Entrée, pas de pull-up, pas de pull-down).
    // https://github.com/peterharperuk/pico-examples/blob/powman/powman/powman_timer/powman_timer.c
    gpio_set_dir_all_bits(0);
    // Les GPIO 0 ET sont affectées à l'UART. laisse pour éventuel debug.
    for (int i = 2; i < NUM_BANK0_GPIOS; ++i)
    {
        gpio_set_function(i, GPIO_FUNC_SIO);
        if (i > NUM_BANK0_GPIOS - NUM_ADC_CHANNELS)
        {
            gpio_disable_pulls(i);
            gpio_set_input_enabled(i, false);
        }
    }
    if (!debug)
    {
        printf("\n[powerDownAndReboot] Entrée en Power Down pour %d s. Au revoir...\n", delay_s);
        stdio_flush();

        // On calcule l'échéance.
        uint64_t now = powman_timer_get_ms();
        uint64_t alarm_time = now + (delay_s * 1000);

        // On arme le réveil.
        powman_enable_alarm_wakeup_at_ms(alarm_time);
        printf("[powerDownAndReboot] Timer démarré à %llu ms. Réveil à %llu ms.\n", now, alarm_time);
        stdio_flush();

        // On prépare la coupure de l'alim.
        printf("[powerDownAndReboot] Init Init de l'etat de power down.\n");
        stdio_flush();

        powman_power_state P1_7 = POWMAN_POWER_STATE_NONE;
        P1_7 = powman_power_state_with_domain_off(P1_7, POWMAN_POWER_DOMAIN_SRAM_BANK1);    // bank1 includes the top 256K of sram plus sram 8 and 9 (scratch x and scratch y)
        P1_7 = powman_power_state_with_domain_off(P1_7, POWMAN_POWER_DOMAIN_SRAM_BANK0);    // bank0 is bottom 256K of sSRAM
        P1_7 = powman_power_state_with_domain_off(P1_7, POWMAN_POWER_DOMAIN_XIP_CACHE);     // XIP cache is 2x8K instances
        P1_7 = powman_power_state_with_domain_off(P1_7, POWMAN_POWER_DOMAIN_SWITCHED_CORE); // Switched core logic (processors, busfabric, peris etc)
        int status = powman_set_power_state(P1_7);
        if (status != PICO_OK)
        {
            printf(" Erreur powman_set_power_state = %d\n", status);
            stdio_flush();
        }
        printf("[powerDownAndReboot] Power Down imminent. Réveil dans %u s\n", delay_s);
        stdio_flush();

        // On éteint le processeur.
        __asm volatile("wfi");

        // Si le Power State a bien coupé les coeurs, on ne verra jamais cette ligne.
        while (1)
            ;
    }
    else
    {
        gpio_init(LED_NODE_PIN);
        gpio_set_dir(LED_NODE_PIN, GPIO_OUT);
        for (int i = delay_s; i > 0; i--)
        {
            printf("Rebbot dans %ds...\n", i);
            for (int i = 5; i > 0; i--)
            {
                gpio_put(LED_NODE_PIN, 1); // Allumer
                sleep_ms(100);             // Attendre 100ms

                gpio_put(LED_NODE_PIN, 0); // Eteindre
                sleep_ms(100);             // Attendre 100ms
            }
        }
        // On active le watchdog avec un délai très court
        // 1ms est le minimum.
        watchdog_reboot(0, 0, 10);

        // On attend la fin
        while (1)
            ;
    }
}
// On définit les résultats possibles de l'attente
enum class AckStatus
{
    OK,
    TIMEOUT,
    ERREUR
};

AckStatus waitForAckLowPower(uint32_t timeout_ms, LoRaHeader &outAck, LoRaNodeIdType nodeIdType)
{
    AckStatus returnValue = AckStatus::TIMEOUT;
    timeout_expired = false;

    // Initialisation de l'alarme.
    // On demande au timer de nous appeler dans timeout_ms.
    alarm_id_t alarm_id = add_alarm_in_ms(timeout_ms, alarm_callback, NULL, false);

    // Les it sont déjà initialisé dans LoRa pour la réception.

    do
    {

        // Dodo et economie d'énergie!
        __asm volatile("wfi");

        // Au réveil, on regarde si c'est le LoRa.
        int res = LoRa.lora_event((uint8_t *)&outAck);
        if (res > 0)
        {
            // C'est un message LoRa.
            if ((outAck.msgType == LoRaMsgType::ACK) &&
                (outAck.dstNodeID == nodeIdType))
            {
                // On est bon,c'est bien un Ack qui nous est destiné.
                cancel_alarm(alarm_id); // On annule l'alarme si on a reçu l'Ack !
                returnValue = AckStatus::OK;
            }
        }
        else
        {
            // Regarde si c'est le timer.
            if (timeout_expired == true)
            {
                // Révéillé par le time out.
                returnValue = AckStatus::TIMEOUT;
            }
        }
        printf("Sortie du wfi! \n");
    } while ((returnValue != AckStatus::OK) &&
             (returnValue != AckStatus::TIMEOUT));

    return returnValue;
}

AckStatus sendAck(LoRaHeader &outAck)
{
    AckStatus returnValue = AckStatus::ERREUR;
    int result = LoRa.sendPacketBlocking((const uint8_t *)&outAck, sizeof(LoRaHeader));
    if (result > 0)
    {
        printf("Message Ack envoye avec succes. Taille: %d / %d bytes\n",
               result, sizeof(LoRaHeader));
        returnValue = AckStatus::OK;
    }
    else
    {
        printf("Erreur lors de l'envoi du message Ack. Code d'erreur: %d\n",
               result);
        returnValue = AckStatus::ERREUR;
    }
    // Repasser en mode réception continue.
    LoRa.rxContinuous();

    return returnValue;
}

#if 0
AckStatus waitForAck(uint32_t timeout_ms, LoRaHeader &outAck)
{
    // Attente en économie d'énergie
    AckStatus returnValue = AckStatus::ERREUR;

    if (returnValue == AckStatus::OK)
    {
        // On a reçu quelque chose.
        int res = LoRa.lora_event((uint8_t *)&outAck);

        // On a reçu un message!
        if (res > 0)
        {
            if ((outAck.msgType == LoRaMsgType::ACK) &&
                (outAck.dstNodeID == LoRaNodeIdType::TEST_DIALOG_LORA))
            {
                // On est bon si c'est bien un Ack qui nous est destiné.
                returnValue = AckStatus::OK;
            }
        }
    }
    return returnValue;
}
#endif

// Fonction principale pour le mode node.
//=======================================
void runForNodeMode()
{

    // Configurer la puissance de transmission (17 dBm)
    LoRa.setTxPower(17);

    // Buffer pour le message/
    TestDialogLoraMsg message;

    // Buffer pour l'Ack
    LoRaHeader receivedAck;

    // Compteur d'occurrence
    uint32_t noOccurenceLocal = 0;
    uint32_t precedentSNR = -32;

    /// init des champs fixes du Header.
    printf("Taille du message TestDialogLoraMsg=%d\n", sizeof(TestDialogLoraMsg));

    message.header.srcNodeID = LoRaNodeIdType ::TEST_DIALOG_LORA;
    message.header.dstNodeID = LoRaNodeIdType ::HUB;
    message.header.msgType = LoRaMsgType::TEST_DIALOG_LORA;
    message.header.reserved = 0;

    // Boucle principale
    while (true)
    {
        printf("Mode Node active\n");

        // Construire le message avec le numéro d'occurrence.
        //---------------------------------------------------
        // récupère le no d'occurence dans la zone sauvegardée.
        noOccurenceLocal = powman_hw->scratch[0];
        // Mise à jour du message et incrémentation.
        message.header.seqNo = noOccurenceLocal++;
        // Sauvegarder immédiatement pour le prochain coup.
        powman_hw->scratch[0] = noOccurenceLocal;
        message.header.prevSNR = precedentSNR;
        message.solarCurrent = 111;
        message.batteryMv = 222;
        message.temperature = 33.3;

        // Envoyer le message.
        //--------------------
        printf("Envoi du message: TestDialogLoraMsg\n");
        gpio_put(LED_NODE_PIN, 1); // Allumer.
        int result = LoRa.sendPacketBlocking((const uint8_t *)&message, sizeof(TestDialogLoraMsg));
        // Passer en mode réception continue le plus vite possible pour ne pas anquer l'Ack.
        LoRa.rxContinuous();
        if (result > 0)
        {
            //            printf("Message #%lu envoye avec succes. Taille: %d bytes\n", noOccurenceLocal, result);
        }
        else
        {
            printf("Erreur lors de l'envoi du message #%lu. Code d'erreur: %d\n", noOccurenceLocal, result);
        }

        // Attente de l'Ack.
        //------------------
        LoRaHeader receivedAck;
        //        printf("Attente de l'Ack du Hub.\n");
        AckStatus status = waitForAckLowPower(2000, receivedAck, LoRaNodeIdType::TEST_DIALOG_LORA); // Attend 2 secondes

        switch (status)
        {
        case AckStatus::OK:
            printf("ACK reçu !\n");
            printf(" RSSI: %d dBm SNR: %f \n", LoRa.packetRssi(), LoRa.packetSnr());
            precedentSNR = std::clamp((int)receivedAck.prevSNR, -32, 31);
            printf("precedentSNR  %ddB\n", precedentSNR);
            break;
        case AckStatus::TIMEOUT:
            printf("Erreur : Timeout (Pas de réponse du Hub)\n");
            break;
        default:
            printf("Erreur de réception : %d\n", (int)status);
            break;
        }

        // On s'endort pour 10s et on reboot.
        powerDownAndReboot(10, true);
    }
}

// Fonction principale pour le mode Hub.
//======================================
void runForHubMode()
{
    printf("Mode HUB active\n");

    gpio_put(LED_HUB_PIN, 0); // Eteint la led.

    // Configurer la puissance de transmission (17 dBm)
    LoRa.setTxPower(17);

    // Buffer pour recevoir les messages
    uint8_t receivedData[128]; // Taille maximale de 128 bytes

    printf("En attente de messages LoRa...\n");

    // Passer en mode réception continue
    LoRa.rxContinuous();

    // Boucle principale
    while (true)
    {
        // Vérifier si un message a été reçu.
        //-----------------------------------
        int packetSize = LoRa.lora_event(receivedData);

        if (packetSize > 0)
        {
            // Message reçu avec succés.
            //--------------------------
            printf("Message recu (%d bytes): ", packetSize);

            // vérifier que c'est destiné au HUB.
            //-----------------------------------
            if (((LoRaHeader *)receivedData)->dstNodeID == LoRaNodeIdType::HUB)
            {
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

            // Afficher des informations supplémentaires.
            //--------------------------------------------
            printf("RSSI: %d dBm\n", LoRa.packetRssi());
            printf("SNR: %.2f dB\n", LoRa.packetSnr());
            printf("Frequency Error: %ld Hz\n", LoRa.packetFrequencyError());
            printf("\n");

            // Envoie de l'Ack.
            //           sleep_ms(100);

            LoRaHeader ackMsg;
            ackMsg.dstNodeID = ((LoRaHeader *)receivedData)->srcNodeID;
            ackMsg.srcNodeID = LoRaNodeIdType::HUB;
            ackMsg.seqNo = ((LoRaHeader *)receivedData)->seqNo;
            ackMsg.msgType = LoRaMsgType::ACK;
            ackMsg.prevSNR = LoRa.packetSnr();

            AckStatus status = sendAck(ackMsg);
            if (status == AckStatus::OK)
            {
                printf("Message Ack envoyé! \n");
            }
            else
            {
                printf("Pb d'envoie du message Ack! \n");
            }
        }
        else if (packetSize == -2)
        {
            // Erreur de CRC
            printf("Message recu avec erreur de CRC\n");
        }
        else if (packetSize == -3)
        {
            // Message trop grand
            printf("Message trop grand pour le buffer\n");
        }

        // Petite pause pour éviter de saturer le CPU
        sleep_ms(100);
        // gpio_put(LED_HUB_PIN, 0); // Eteint la led.
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
        printf("Demarrage dans %ds...\n", i);

        gpio_put(LED_NODE_PIN, 1); // Allumer
        gpio_put(LED_HUB_PIN, 1);  // Allumer
        sleep_ms(500);             // Attendre 500ms

        gpio_put(LED_NODE_PIN, 0); // Eteindre
        gpio_put(LED_HUB_PIN, 0);  // Eteindre
        sleep_ms(500);             // Attendre 500ms
    }

    // init de la pesrsistance du compteur d'occurence pour le node.
    if (powman_hw->scratch[1] != 0xDEADBEEF) // Valeur improbable à la première utilisation du processeur.
    {
        powman_hw->scratch[0] = 0; // Initialisation premier démarrage
        powman_hw->scratch[1] = 0xDEADBEEF;
    }

    // On dit au Power Manager d'utiliser l'oscillateur basse consommation.
    powman_timer_set_1khz_tick_source_lposc();

    // Attendre un court instant que l'oscillateur soit stable.
    sleep_ms(1);

    // 1. On demerre le timer du powman.
    powman_timer_start();

    printf("Initialisation du module LoRa...\n");

    // Initialiser le module LoRa
    if (!LoRa.init())
    {
        printf("Erreur d'initialisation du module LoRa!\n");
        return -1;
    }

    printf("Module LoRa initialise avec succes.\n");

    // Choisir le mode en fonction de la clÃ© de compilation
    switch (LORA_MODE)
    {
    case LORA_NODE:
        runForNodeMode();
        break;
    case LORA_HUB:
        runForHubMode();
        break;
    default:
        printf("Mode non valide.\n");
        return -1;
    }

    return 0;
}