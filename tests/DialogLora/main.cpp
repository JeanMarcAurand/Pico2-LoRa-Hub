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

#include "pico/rand.h"

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

const uint LED_HUB_PIN = 4;   // pour pico W, cabler la led!
const uint LED_NODE_PIN = 25; // pour pico sans W, led intédrée.

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
#include <cmath>

class PersistantParams
{
private:
    enum class IndexOfPersistantsParams : uint32_t
    {
        NO_OCCURENCE = 0,
        PREV_SNR = 1,
        PREV_RSSI = 2,
        TX_POWER = 3,
        NB_MISSING_ACK = 4,
        MAGIC_KEY = 5
    };

    static const uint32_t MAGIC_VALUE = 0xDEADBEEF;

    // Helper interne pour le cast de l'index
    static inline uint32_t idx(IndexOfPersistantsParams i)
    {
        return static_cast<uint32_t>(i);
    }

public:
    static void init()
    {
        if (powman_hw->scratch[idx(IndexOfPersistantsParams::MAGIC_KEY)] != MAGIC_VALUE)
        {
            // Peu probable de trouver cette valeur à la mise sous tension.
            // En revanche sera maintenu suite à un reboot.
            setNoOccurrence(0);
            setPrevSNR(-32);
            setPrevRSSI(-200);
            setTxPower(17); // Puissance max par défaut
            setNbMissingAck(0);
            powman_hw->scratch[idx(IndexOfPersistantsParams::MAGIC_KEY)] = MAGIC_VALUE;
        }
    }

    // Gestion du SNR (signé int32_t)
    static void setPrevSNR(int32_t snr)
    {
        powman_hw->scratch[idx(IndexOfPersistantsParams::PREV_SNR)] = static_cast<uint32_t>(snr);
    }

    static int32_t getPrevSNR()
    {
        return static_cast<int32_t>(powman_hw->scratch[idx(IndexOfPersistantsParams::PREV_SNR)]);
    }

    // Gestion du RSSI (signé int32_t)
    static void setPrevRSSI(int32_t rssi)
    {
        powman_hw->scratch[idx(IndexOfPersistantsParams::PREV_RSSI)] = static_cast<uint32_t>(rssi);
    }

    static int32_t getPrevRSSI()
    {
        return static_cast<int32_t>(powman_hw->scratch[idx(IndexOfPersistantsParams::PREV_RSSI)]);
    }

    // Gestion de la Puissance (uint32_t)
    static void setTxPower(uint32_t power)
    {
        powman_hw->scratch[idx(IndexOfPersistantsParams::TX_POWER)] = power;
    }

    static uint32_t getTxPower()
    {
        return powman_hw->scratch[idx(IndexOfPersistantsParams::TX_POWER)];
    }

    // Gestion du d'ack perdu (uint32_t)
    static void setNbMissingAck(uint32_t nbMissingAck)
    {
        powman_hw->scratch[idx(IndexOfPersistantsParams::NB_MISSING_ACK)] = nbMissingAck;
    }

    static uint32_t getNbMissingAck()
    {
        return powman_hw->scratch[idx(IndexOfPersistantsParams::NB_MISSING_ACK)];
    }

    // Gestion du noOccurrence (uint32_t)
    static void setNoOccurrence(uint32_t noOccurrence)
    {
        powman_hw->scratch[idx(IndexOfPersistantsParams::NO_OCCURENCE)] = noOccurrence;
    }

    static uint32_t getNoOccurrence()
    {
        return powman_hw->scratch[idx(IndexOfPersistantsParams::NO_OCCURENCE)];
    }
};

void powerDownAndReboot_ms(uint32_t delay_ms, bool debug)
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
    printf("\n[powerDownAndReboot] Entrée en Power Down pour %d ms. Au revoir...\n", delay_ms);
    if (!debug)
    {
        stdio_flush();

        // On calcule l'échéance.
        uint64_t now = powman_timer_get_ms();
        uint64_t alarm_time = now + delay_ms;

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
        printf("[powerDownAndReboot] Power Down imminent. Réveil dans %u ms\n", delay_ms);
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
        for (int i = delay_ms / 1000; i > 0; i--)
        {
            printf("Rebbot dans %ds...  \r", i);
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
        //        printf("Sortie du wfi! \n");
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
/*
 * =========================================================================
 * TABLE 13 : Gamme des Facteurs d'Étalement (Spreading Factors - SF) page 27
 * =========================================================================
 * SpreadingFactor | Facteur d'étalement  | SNR du démodulateur
 *(RegModemConfig2)|   (Chips / symbole)  |      LoRa (dB)
 * -------------------------------------------------------------------------
 *        6        |          64          |       -5    dB
 *        7        |         128          |       -7.5  dB
 *        8        |         256          |      -10    dB
 *        9        |         512          |      -12.5  dB
 *       10        |        1024          |      -15    dB
 *       11        |        2048          |      -17.5  dB
 *       12        |        4096          |      -20    dB
 * =========================================================================
 * Note : Le SF6 est un cas particulier nécessitant un header implicite [1].
 * Plus le SF est élevé, plus la sensibilité (portée) augmente au détriment
 * du débit binaire [3].
 */
#define LIMITE_SNR_POUR_SF -7.5 // dB cas SF=7 (SPREADING_FACTOR) cf table ci-dessus
#define MARGE_SNR 5             // dB.

/**
 * ============================================================================
 * SENSIBILITÉ BRUTE LoRa - CAS LOW FREQUENCY (LF : Bandes 2 & 3) [dBm]
 * Source : SX1276/77/78/79 Datasheet - Table 10 (Electrical Specifications) page 20
 * ============================================================================
 * SF \ BW | 7.8 kHz | 10.4 kHz | 62.5 kHz | 125 kHz | 250 kHz | 500 kHz
 * ----------------------------------------------------------------------------
 *  SF6    |    --   |   -132   |   -123   |   -121  |   -118  |   -112
 *  SF7    |    --   |   -136   |   -128   |   -125  |   -122  |   -118
 *  SF8    |    --   |   -138   |   -131   |   -128  |   -125  |   -121
 *  SF9    |    --   |    --    |   -134   |   -131  |   -128  |   -124
 *  SF10   |    --   |    --    |   -135   |   -134  |   -131  |   -127
 *  SF11   |   -145  |    --    |   -137   |   -136  |   -133  |   -129
 *  SF12   |   -148  |    --    |   -140   |   -137  |   -134  |   -130
 * ============================================================================
 * Note : Les cases "--" indiquent des valeurs non spécifiées dans la table 10.
 * La sensibilité augmente avec le Spreading Factor (SF) et diminue avec
 * l'augmentation de la bande passante (BW).
 */
#define LIMITE_RSSI_POUR_SF_BW -125 // dB cas SF=7 (SPREADING_FACTOR) bande=125kHz (BANDWIDTH).
#define MARGE_RSSI 10               // dB.

/*
 Si   SNR  < (LIMITE_SNR_POUR_SF+MARGE_SNR)
   ou RSSI < (LIMITE_RSSI_POUR_SF_BW+MARGE_RSSI)
alors puissance à la hausse
sinon
     si   SNR  > (LIMITE_SNR_POUR_SF+MARGE_SNR)
       et RSSI > (LIMITE_RSSI_POUR_SF_BW+MARGE_RSSI)
    alors puissance à la baisse
*/
int computeNextTxPower(int precedentSNR, int precedentRSSI, int currentTxPower)
{
    printf("[--------------------\n");
    int nextTxPower; // entre 2 et 17 dBm.
    int deltaSnr = precedentSNR - (LIMITE_SNR_POUR_SF + MARGE_SNR);
    int deltaRssi = precedentRSSI - (LIMITE_RSSI_POUR_SF_BW + MARGE_RSSI);
    printf(" precedentSNR = %d deltaSNR=%d\n", precedentSNR, deltaSnr);
    printf(" precedentRSSI = %d deltaRssi=%d\n", precedentRSSI, deltaRssi);
    printf(" currentTxPower = %d \n", currentTxPower);

    if ((deltaSnr < 0) || (deltaRssi < 0))
    {
        // On augmente la puissance.
        if (deltaSnr < deltaRssi)
        {
            nextTxPower = ceil(currentTxPower - deltaSnr * 1.3);
        }
        else
        {
            nextTxPower = ceil(currentTxPower - deltaRssi * 1.3);
        }
        printf(" On augmente la puissance nextTxPower= %d \n", nextTxPower);
    }
    else
    {
        // On diminue la puissance.
        if (deltaSnr < deltaRssi)
        {
            nextTxPower = ceil(currentTxPower - deltaSnr * 0.5);
        }
        else
        {
            nextTxPower = ceil(currentTxPower - deltaRssi * 0.5);
        }
        printf(" On diminue la puissance nextTxPower= %d \n", nextTxPower);
    }
    nextTxPower = std::clamp(nextTxPower, 2, 17);
    // Si l'ecart de correction est trop petit, ne fait rien.
    if (abs(nextTxPower - currentTxPower) <= 2)
    {
        nextTxPower = currentTxPower;
    }
    printf("--------------------]\n");

    return nextTxPower;
}

// Fonction principale pour le mode node.
//=======================================
void runForNodeMode()
{
    // Init des paramètre pesrsistants.
    PersistantParams persistantParams;
    persistantParams.init();

    // Durée de sommeil.
    const int recurrence_ms = 10000;
    // Jitter de la recurrence en ms.
    int jitter = 0;

    // Buffer pour le message/
    TestDialogLoraMsg message;

    // Buffer pour l'Ack
    LoRaHeader receivedAck;

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
        {
            // récupère le no d'occurence dans la zone sauvegardée.
            uint32_t noOccurenceLocal = persistantParams.getNoOccurrence();
            // Mise à jour du message et incrémentation.
            message.header.seqNo = noOccurenceLocal++;
            // Sauvegarder immédiatement pour le prochain coup.
            persistantParams.setNoOccurrence(noOccurenceLocal);
        }
        message.header.prevSNR = persistantParams.getPrevSNR();
        message.header.prevRSSI = persistantParams.getPrevRSSI();
        message.solarCurrent = 111;
        message.batteryMv = 222;
        message.temperature = 33.3;

        // Envoyer le message.
        //--------------------
        printf("Envoi du message: TestDialogLoraMsg (%ddB)\n", persistantParams.getTxPower());
        gpio_put(LED_NODE_PIN, 1); // Allumer.

        // Configurer la puissance de transmission.
        LoRa.setTxPower(persistantParams.getTxPower());

        // Envoie du message.
        int result = LoRa.sendPacketBlocking((const uint8_t *)&message, sizeof(TestDialogLoraMsg));

        // Passer en mode réception continue le plus vite possible pour ne pas anquer l'Ack.
        LoRa.rxContinuous();
        if (result > 0)
        {
            //            printf("Message #%lu envoye avec succes. Taille: %d bytes\n", noOccurenceLocal, result);
        }
        else
        {
            printf("Erreur lors de l'envoi du message #%lu. Code d'erreur: %d\n",
                   (persistantParams.getNoOccurrence() - 1), result);
        }

        // Attente de l'Ack.
        //------------------
        LoRaHeader receivedAck;
        //        printf("Attente de l'Ack du Hub.\n");
        AckStatus status = waitForAckLowPower(2000, receivedAck, LoRaNodeIdType::TEST_DIALOG_LORA); // Attend 2 secondes

        switch (status)
        {
        case AckStatus::OK:
            printf("ACK reçu ! RSSI: %d dBm SNR: %fdB\n", LoRa.packetRssi(), LoRa.packetSnr());
            // Correspond au SNR pour un message envoye par le Hub et mesuré sur le Node.
            // Sera retourné au Hub pour ajuster sa puissance d'emission dans le cas de ce noeud.
            persistantParams.setPrevSNR(std::clamp((int)LoRa.packetSnr(), -32, 31));
            persistantParams.setPrevRSSI(std::clamp((int)LoRa.packetRssi(), -150, 10));
            persistantParams.setTxPower(computeNextTxPower(receivedAck.prevSNR, receivedAck.prevRSSI, persistantParams.getTxPower()));
            persistantParams.setNbMissingAck(0);
            break;
        case AckStatus::TIMEOUT:
        {
            printf("Erreur : Timeout (Pas de réponse du Hub)\n");
            // On fait jitter la recurrence entre 0 et 10% recurence.
            jitter = get_rand_32() % (recurrence_ms / 10);

            uint32_t nombreAckPerdu = persistantParams.getNbMissingAck() + 1;
            persistantParams.setNbMissingAck(nombreAckPerdu);
            if (nombreAckPerdu == 1)
            {
                // On ne modifie pas la puissance.
                // On fait jitter la recurrence entre 0 et 10% recurence.
            }
            else if (nombreAckPerdu == 2)
            {
                // On ne remonte la puissace que de 6dB et on fait jitter la recurrence.
                persistantParams.setTxPower(std::clamp((int)(persistantParams.getTxPower() + 6), 2, 17));
                // On fait jitter la recurrence entre 0 et 10% recurence.
            }
            else
            {
                // On a perdu la liason, repart en puissance max si la cause est que le Hub n'a pas reçu le message.
                persistantParams.setTxPower(17);
                // On fait jitter la recurrence entre 0 et 10% recurence.
            }
        }
        break;
        default:
            printf("Erreur de réception : %d\n", (int)status);
            // Réinit par précaution.
            persistantParams.setTxPower(17);
            break;
        }

        // On s'endort pour 10s et on reboot.
        powerDownAndReboot_ms(recurrence_ms + jitter, true);
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
            ackMsg.prevRSSI = LoRa.packetRssi();

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