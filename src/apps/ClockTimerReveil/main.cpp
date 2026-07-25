
#include <stdio.h>
#include <algorithm>
#include "pico/stdlib.h"

#include <inttypes.h> // Indispensable pour les macros PRI

#include "hardware/gpio.h"
#include "ClockTimerMsg.h"

#include "pico/util/datetime.h"
#include <ctime>

#include <LoRa.h>
#include <LoRaLink.h>

#include <CurrentTimeMsg.h>
#include <TM1637Display.h>

const uint LED_NODE_PIN = 25;       // pour pico sans W, led intégrée.
const uint ALARME_COMMANDE_PIN = 8; // pour pico sans W, led intégrée.

// Instance de la classe LoRa
LoRaClass loRa;
LoRaLink *loRaLink;

// Instance de la clees pour afficheur.
TM1637Display display(2 /*CLK*/, 3 /*DIO*/);
static volatile bool demiSecond = false;

bool repeatingTimerCallback0_5second(repeating_timer_t *rt)
{
    demiSecond = true;
    return (true); // Pour que le timer continue.
}
void alarmeOn()
{
    gpio_put(ALARME_COMMANDE_PIN, 1);
}
void alarmeOff()
{
    gpio_put(ALARME_COMMANDE_PIN, 0);
}
// Offset pour passer du compteur
uint64_t offset = 0;
uint64_t cptAlarmeDebut_us = 0;
uint64_t cptAlarmeFin_us = 0;
ClockTimerData clockTimerData;

enum class EtatAlarm : uint8_t
{
    ALARME_ON,
    ALARME_OFF
};
EtatAlarm etatAlarm = EtatAlarm::ALARME_OFF;

void defaultInitClockTimerData(void)
{
    clockTimerData.alarmMode = (u_int32_t)ClockTimerAlarm::ARRET;
    clockTimerData.hAlarme = 0;
    clockTimerData.mAlarme = 0;
    clockTimerData.sAlarme = 0;
    clockTimerData.duree = 0;
}

uint64_t calculeOffset(const CurrentTimeData *currentTimeData)
{

    struct tm t;
    t.tm_year = currentTimeData->annee - 1900; // Année depuis 1900
    t.tm_mon = currentTimeData->mois;
    t.tm_mday = currentTimeData->jour;
    t.tm_hour = currentTimeData->heure;
    t.tm_min = currentTimeData->minute;
    t.tm_sec = currentTimeData->seconde;
    // Calcul de l'offset:
    return ((uint64_t)mktime(&t) * 1000000ULL - to_us_since_boot(get_absolute_time()));
}
void calculeCompteurDebutFin(const uint64_t offset, const ClockTimerData *clockTimerData,
                             uint64_t *ptCptAlarmeDebut_us, uint64_t *ptCptAlarmeFin_us)
{
    // Calcul de l'heure de l'alarme référence cpt pico pour la journée courante.
    // Determine la date de la journée courante.
    {
        uint64_t localCurrentPicotime_us = to_us_since_boot(get_absolute_time());

        time_t timestamp = (time_t)((localCurrentPicotime_us + offset) / 1000000ULL);
        struct tm *ptm;
        ptm = gmtime(&timestamp);
        if (ptm != nullptr)
        {
            // Met a jour l'heure.
            struct tm heureDebutAlarme;
            memcpy(&heureDebutAlarme, ptm, sizeof(tm));
            heureDebutAlarme.tm_hour = clockTimerData->hAlarme;
            heureDebutAlarme.tm_min = clockTimerData->mAlarme;
            heureDebutAlarme.tm_sec = clockTimerData->sAlarme;
            *ptCptAlarmeDebut_us = (uint64_t)mktime(&heureDebutAlarme) * 1000000ULL - offset;
            if (cptAlarmeDebut_us < localCurrentPicotime_us)
            {
                // On ajoute 24h pour viser demain.
                cptAlarmeDebut_us += (24 * 60 * 60 * 1000000ULL);
            }
            *ptCptAlarmeFin_us = *ptCptAlarmeDebut_us + clockTimerData->duree * 1000000ULL;
        }
        printf("localCurrentPicotime_us= %" PRIx64 " *ptCptAlarmeDebut_us=%" PRIx64 " *ptCptAlarmeFin_us=%" PRIx64 "\n ",
               localCurrentPicotime_us, *ptCptAlarmeDebut_us, *ptCptAlarmeFin_us);
    }
}
LoRaLink::AckStatus sendLoraBootRequest(void)
{
    // Construction du message de demande de boot.
    LoRaHeader loRaHeader; // La demande d'heure est juste un header ave TIME_REQUEST.
    loRaHeader.srcNodeID = LoRaNodeIdType::CLOCK_TIMER;
    loRaHeader.dstNodeID = LoRaNodeIdType::HUB;
    loRaHeader.msgType = LoRaMsgType::BOOT_REQUEST;

    // Envoie du message de boot.
    return (loRaLink->sendLoRaMessage(500, &loRaHeader, sizeof(LoRaHeader)));
}

uint8_t points_visibles = 0;

static const uint8_t clear[] = {0, 0, 0, 0};

static const uint8_t SEG_BOOT[] = {
    0x7C, // b
    0x5C, // o
    0x5C, // o
    0x78  // t
};
// Définition des segments pour "SYnc"
static const uint8_t SEG_SYNC[] = {
    0x6D, // S
    0x6E, // Y
    0x54, // n
    0x58  // c
};

// Définition des segments pour "Done"
static const uint8_t SEG_DONE[] = {
    0x5E, // d (minuscule pour être distinct du 0)
    0x5C, // o (minuscule)
    0x54, // n (minuscule)
    0x79  // E (Majuscule)
};

void afficheAttente(int step)
{

    display.setBrightness(0x02);
    if (step == 0)
    {
        printf(" Done!\n");
        display.setSegments(SEG_DONE);
        sleep_ms(1000);
    }
    else
    {
        printf(" Sync : %d\n", step);
        for (int i = 0; i < 10; i++)
        {
            display.setSegments(SEG_SYNC);
            sleep_ms(500); // Attendre 500ms
            display.showNumberDecEx(step, 0b10000000, true);
            sleep_ms(500); // Attendre 500ms
        }
    }
}

enum class BootState
{
    SEND_BOOT_REQUEST, // Envoi du message "Je suis là, j'ai besoin de tout"
    WAIT_FOR_TIME,     // Attente du message CurrentTimeData
    WAIT_FOR_ALARM,    // Attente du message ClockTimerData
    BOOT_COMPLETE      // Prêt à fonctionner
};

BootState currentBootState = BootState::SEND_BOOT_REQUEST;
void nodeBoot(void)
{
    printf("nodeBoot: Demarrage du boot...\n");
    display.setBrightness(0x02);
    display.setSegments(SEG_BOOT);
    sleep_ms(1000);

    while (currentBootState != BootState::BOOT_COMPLETE)
    {
        LoRaLink::AckStatus status = LoRaLink::AckStatus::ERREUR;

        switch (currentBootState)
        {
        case BootState::SEND_BOOT_REQUEST:
        {
            printf("nodeBoot: Envoi de la demande de configuration...\n");
            status = sendLoraBootRequest();
            if ((status == LoRaLink::AckStatus::OK) ||
                (status == LoRaLink::AckStatus::LOST_MESSAGE))
            {
                // On est bon, on se met en attente de l'heure courante.
                currentBootState = BootState::WAIT_FOR_TIME;
            }
            else
            {
                printf("nodeBoot: Problème envoie boot request. Status attendu= %d (LoRaLink::AckStatus::OK) recu %d\n",
                       LoRaLink::AckStatus::OK, status);
                // On temporise un peu avant de faire une nouvelle demande.
                afficheAttente(1);
            }
            break;
        }
        case BootState::WAIT_FOR_TIME:
        {
            // Se met en attente du message contenant l'heure courante.
            uint8_t message[128];
            status = loRaLink->waitForReceiveLowPower(1000, message, LoRaNodeIdType::CLOCK_TIMER, LoRaNodeIdType::HUB);
            if ((status == LoRaLink::AckStatus::OK) ||
                (status == LoRaLink::AckStatus::LOST_MESSAGE))
            {
                LoRaHeader *loRaHeader = (LoRaHeader *)message;
                // Verifie que c'est bien un message de mise à l'heure.
                if (loRaHeader->msgType == LoRaMsgType::CURRENT_TIME)
                {
                    // On envoie l'Ack.
                    status = loRaLink->sendAck(loRaHeader);
                    if (status != LoRaLink::AckStatus::OK)
                    {
                        printf("nodeBoot: Problème envoie Ack. Status attendu= %d (LoRaLink::AckStatus::OK) recu %d\n",
                               LoRaLink::AckStatus::OK, status);
                    }
                    // Même si l'envoie de l'ack n'est pas OK, prend en compte le message.
                    // Met a jour l'offset pour lien entre cpt pico et heure courante.
                    offset = calculeOffset((CurrentTimeData *)message);

                    // Tout est correct, on peut se mettre en attente du message d'alarme.
                    currentBootState = BootState::WAIT_FOR_ALARM;
                }
                else
                {
                    printf("nodeBoot: Invalid message Id. Attendu %d(LoRaMsgType::CURRENT_TIME) recu %d!\n",
                           LoRaMsgType::CURRENT_TIME, loRaHeader->msgType);
                    // On refait une demande boot request.
                    currentBootState = BootState::SEND_BOOT_REQUEST;
                    afficheAttente(2);
                }
            }
            else
            {
                // Pas recu le message (timeout).
                printf("nodeBoot: Time out reception heure courante!\n");
                // On refait une demande boot request.
                currentBootState = BootState::SEND_BOOT_REQUEST;
                afficheAttente(2);
            }
            break;
        }
        case BootState::WAIT_FOR_ALARM:
        {
            // Attente du message d'alarme.
            uint8_t message[128];
            status = loRaLink->waitForReceiveLowPower(1000, message, LoRaNodeIdType::CLOCK_TIMER, LoRaNodeIdType::HUB);
            if ((status == LoRaLink::AckStatus::OK) ||
                (status == LoRaLink::AckStatus::LOST_MESSAGE))
            {
                LoRaHeader *loRaHeader = (LoRaHeader *)message;
                // Regarde si c'est un message de mise à jour de l'alarme.
                if (loRaHeader->msgType == LoRaMsgType::CLOCK_TIMER)
                {
                    // On envoie l'Ack.
                    status = loRaLink->sendAck(loRaHeader);
                    if (status != LoRaLink::AckStatus::OK)
                    {
                        printf("nodeBoot: Problème envoie Ack. Status attendu= %d (LoRaLink::AckStatus::OK) recu %d\n",
                               LoRaLink::AckStatus::OK, status);
                    }
                    // Même si l'envoie de l'ack n'est pas OK, prend en compte le message.
                    // Stocke les nouveaux paramètres d'alarme.
                    memcpy(&clockTimerData, message, sizeof(ClockTimerData));

                    // Met à jour les compteurs pico.
                    calculeCompteurDebutFin(offset, &clockTimerData,
                                            &cptAlarmeDebut_us, &cptAlarmeFin_us);

                    // Tout est correct, on pa reussi le boot!
                    currentBootState = BootState::BOOT_COMPLETE;
                }
                else
                {
                    printf("nodeBoot: Invalid message Id. Attendu %d(LoRaMsgType::CLOCK_TIMER) recu %d!\n",
                           LoRaMsgType::CLOCK_TIMER, loRaHeader->msgType);
                    // On refait une demande boot request.
                    currentBootState = BootState::SEND_BOOT_REQUEST;
                    afficheAttente(3);
                }
            }
            break;
        }

            tight_loop_contents();
        }

        // Boot réussi!
        afficheAttente(0);
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

    gpio_init(ALARME_COMMANDE_PIN);
    gpio_set_dir(ALARME_COMMANDE_PIN, GPIO_OUT);

#if 0

    int compteur = 0;

    // Boucle infinie pour tests ;-)
    while (true) {
        printf("Blink numero : %d\n", compteur++);
        
        gpio_put(LED_NODE_PIN, 1); // Allumer
         sleep_ms(500);        // Attendre 500ms
        
        gpio_put(LED_NODE_PIN, 0); // Eteindre
        sleep_ms(500);        // Attendre 500ms
    }
#endif
    // Petite pause pour te laisser le temps d'ouvrir le moniteur série
    for (int i = 10; i > 0; i--)
    {
        printf("Demarrage dans %ds...  \r", i);

        gpio_put(LED_NODE_PIN, 1); // Allumer
        alarmeOn();
        sleep_ms(500); // Attendre 500ms

        gpio_put(LED_NODE_PIN, 0); // Eteindre
        alarmeOff();
        sleep_ms(500); // Attendre 500ms
    }

    // On dit au Power Manager d'utiliser l'oscillateur basse consommation.
    powman_timer_set_1khz_tick_source_lposc();

    // Attendre un court instant que l'oscillateur soit stable.
    sleep_ms(1);

    // On demarre le timer du powman.
    powman_timer_start();

    printf("Initialisation du module LoRa...\n");

    // Initialiser le module LoRa
    if (!loRa.init())
    {
        printf("Erreur d'initialisation du module LoRa!\n");
        return -1;
    }
    printf("Module LoRa initialise avec succes.\n");

    // Init pour methode haut niveau.
    loRaLink = new LoRaLink(&loRa, LoRaLink::NodeType::NODE);

    // Configurer la puissance de transmission (17 dBm) max.
    loRa.setTxPower(17);

    // Buffer pour recevoir les messages
    uint8_t receivedData[128]; // Taille maximale de 128 bytes

    // Passer en mode réception continue
    loRa.rxContinuous();

    // Timer à la demie seconde.
    repeating_timer_t out;
    add_repeating_timer_ms(500, repeatingTimerCallback0_5second, nullptr, &out);

    // Alarme par défaut.
    defaultInitClockTimerData();

    // Config afficheur.
    display.setBrightness(0x02);
    display.setSegments(clear);

    // recuperation de l'heure courante et de l'alarme.
    nodeBoot();

    while (true)
    {
        if (demiSecond == true)
        {
            demiSecond = false;
            uint64_t currentPicotime_us = to_us_since_boot(get_absolute_time());
            printf("currentPicotime_us= %" PRIx64 " cptAlarmeDebut_us=%" PRIx64 " cptAlarmeFin_us=%" PRIx64 "\n ",
                   currentPicotime_us, cptAlarmeDebut_us, cptAlarmeFin_us);
            switch (etatAlarm)
            {
            case EtatAlarm::ALARME_OFF:
                if (clockTimerData.alarmMode == (u_int32_t)ClockTimerAlarm::RECURRENT)
                {
                    if (currentPicotime_us >= cptAlarmeDebut_us)
                    {
                        // Debut de l'alarme.
                        alarmeOn();
                        etatAlarm = EtatAlarm::ALARME_ON;
                    }
                }
                if ((clockTimerData.alarmMode == (u_int32_t)ClockTimerAlarm::FORCE) ||
                    (clockTimerData.alarmMode == (u_int32_t)ClockTimerAlarm::FORCE_TIMEOUT))
                {
                    // Debut de l'alarme.
                    alarmeOn();
                    etatAlarm = EtatAlarm::ALARME_ON;
                }
                break;
            case EtatAlarm::ALARME_ON:
                if (clockTimerData.alarmMode == (u_int32_t)ClockTimerAlarm::ARRET)
                {
                    // Arret de l'alarme.
                    alarmeOff();
                    etatAlarm = EtatAlarm::ALARME_OFF;
                }
                if ((clockTimerData.alarmMode == (u_int32_t)ClockTimerAlarm::RECURRENT) ||
                    (clockTimerData.alarmMode == (u_int32_t)ClockTimerAlarm::UNE_FOIS) ||
                    (clockTimerData.alarmMode == (u_int32_t)ClockTimerAlarm::FORCE_TIMEOUT))
                {
                    if (currentPicotime_us >= cptAlarmeFin_us)
                    {
                        // Arret de l'alarme.
                        alarmeOff();
                        etatAlarm = EtatAlarm::ALARME_OFF;
                        if (clockTimerData.alarmMode == (u_int32_t)ClockTimerAlarm::RECURRENT)
                        {
                            // On decale l'heure de début d'alarme de 24h
                            // cptAlarmeDebut_us += (24 * 60 * 60 * 1000000ULL);
                            // cptAlarmeFin_us += (24 * 60 * 60 * 1000000ULL);
                            cptAlarmeDebut_us += (30 * 1000000ULL);
                            cptAlarmeFin_us += (30 * 1000000ULL);
                        }
                        else if ((clockTimerData.alarmMode == (u_int32_t)ClockTimerAlarm::UNE_FOIS) ||
                                 (clockTimerData.alarmMode == (u_int32_t)ClockTimerAlarm::FORCE_TIMEOUT))
                        {
                            clockTimerData.alarmMode = (u_int32_t)ClockTimerAlarm::ARRET;
                        }
                    }
                }
                break;

            default:
                break;
            }
            time_t timestamp = (time_t)((currentPicotime_us + offset) / 1000000ULL);
            struct tm *ptm;
            ptm = gmtime(&timestamp);
            if (ptm != nullptr)
            {
                if (points_visibles == 0)
                {
                    points_visibles = 0b10000000;
                    if (etatAlarm == EtatAlarm::ALARME_ON)
                        display.setBrightness(0x00);
                }
                else
                {
                    points_visibles = 0;
                    if (etatAlarm == EtatAlarm::ALARME_ON)
                        display.setBrightness(0x02);
                }
                int valueToDisplay = ptm->tm_min * 100 + ptm->tm_sec;
                if (valueToDisplay < 1000)
                {
                    display.showNumberDecEx(valueToDisplay, points_visibles, true, 3, 1);
                }
                else
                {
                    display.showNumberDecEx(valueToDisplay, points_visibles, true);
                }
            }
        }
        // Regarde si on a recu un message LoRa.
        uint8_t message[128];
        LoRaLink::AckStatus status;

        status = loRaLink->noBlockReceive(message, LoRaNodeIdType::CLOCK_TIMER);
        if ((status == LoRaLink::AckStatus::OK) ||
            (status == LoRaLink::AckStatus::LOST_MESSAGE))
        {
            LoRaHeader *loRaHeader = (LoRaHeader *)message;
            // Regarde si c'est un message de mise à jour de l'alarme.
            if ((loRaHeader->msgType == LoRaMsgType::CLOCK_TIMER) &&
                (loRaHeader->srcNodeID == LoRaNodeIdType::HUB))
            {
                // On envoie l'Ack.
                loRaLink->sendAck(loRaHeader);

                // Stocke les nouveaux paramètres d'alarme.
                memcpy(&clockTimerData, message, sizeof(ClockTimerData));

                // Met à jour les compteurs pico.
                calculeCompteurDebutFin(offset, &clockTimerData,
                                        &cptAlarmeDebut_us, &cptAlarmeFin_us);
            }
            // Regarde si c'est un message broadcast de mise à jour de l'heure courante
            if ((loRaHeader->msgType == LoRaMsgType::CURRENT_TIME) &&
                (loRaHeader->srcNodeID == LoRaNodeIdType::BROADCAST_MSG))
            {
                // Pas de Ack en broadcast.
                // Met a jour l'offset pour lien entre cpt pico et heure courante.
                offset = calculeOffset((CurrentTimeData *)message);
                // Met à jour les compteurs pico avec la nouvelle haure.
                calculeCompteurDebutFin(offset, &clockTimerData,
                                        &cptAlarmeDebut_us, &cptAlarmeFin_us);
            }
        }
        tight_loop_contents();
    }
}
