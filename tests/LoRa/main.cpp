// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <LoRa.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// Instance de la classe LoRa
LoRaClass LoRa;

// Définir le mode: LORA_SEND ou LORA_RECEIVE
// Compilez avec -DLORA_MODE_SEND pour le mode émetteur
// Compilez avec -DLORA_MODE_RECV pour le mode récepteur ou rien par defaut

// Constantes pour les modes
#define LORA_SEND 1
#define LORA_RECEIVE 2

#ifdef LORA_MODE_SEND
#define LORA_MODE LORA_SEND // Mode émetteur
#else
#define LORA_MODE LORA_RECEIVE // Mode par défaut: récepteur
#endif

//    const uint LED_PIN = 4;
const uint LED_PIN = 25;

// Fonction principale pour le mode émetteur
void runForNodeMode()
{

    // Configurer la puissance de transmission (17 dBm)
    LoRa.setTxPower(17);

    // Buffer pour le message
    char message[64];

    // Compteur d'occurrence
    uint32_t counter = 0;

    // Boucle principale
    while (true)
    {
        printf("Mode EMETTEUR LoRa active\n");
        
        // Construire le message avec le numÃ©ro d'occurrence
        int length = snprintf(message, sizeof(message), "Message #%lu: Hello, LoRa!", counter);

        if (length < 0 || length >= sizeof(message))
        {
            printf("Erreur lors de la construction du message\n");
            sleep_ms(1000);
            counter++;
            continue;
        }

        printf("Envoi du message: %s\n", message);
        gpio_put(LED_PIN, 1); // Allumer

        // Envoyer le message
        int result = LoRa.sendPacketBlocking((const uint8_t *)message, length);

        if (result > 0)
        {
            printf("Message #%lu envoye avec succes. Taille: %d bytes\n", counter, result);
        }
        else
        {
            printf("Erreur lors de l'envoi du message #%lu. Code d'erreur: %d\n", counter, result);
        }

        // Incrémenter le compteur
        counter++;

        // Attendre 1 seconde avant le prochain envoi
        sleep_ms(100);        // Attendre 500ms
        gpio_put(LED_PIN, 0); // Eteindre
        sleep_ms(900);
    }
}

// Fonction principale pour le mode rÃ©cepteur
void runForHubMode()
{
    printf("Mode RECEPTEUR LoRa active\n");

    // Configurer la puissance de transmission (17 dBm)
    LoRa.setTxPower(17);

    // Buffer pour recevoir les messages
    uint8_t receivedData[128]; // Taille maximale de 128 bytes

    // Passer en mode réception continue
    LoRa.rxContinuous();

    printf("En attente de messages LoRa...\n");

    // Boucle principale
    while (true)
    {
        // Vérifier si un message a été reçu
        int packetSize = LoRa.lora_event(receivedData);

        if (packetSize > 0)
        {
            // Message reçu avec succés
            printf("Message recu (%d bytes): ", packetSize);

            // Afficher le message
            for (int i = 0; i < packetSize; i++)
            {
                printf("%c", (char)receivedData[i]);
            }
            printf("\n");

            // Afficher des informations supplémentaires
            printf("RSSI: %d dBm\n", LoRa.packetRssi());
            printf("SNR: %.2f dB\n", LoRa.packetSnr());
            printf("Frequency Error: %ld Hz\n", LoRa.packetFrequencyError());
            printf("\n");
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
    }
}

int main()
{
    // Initialiser la carte Pico
    stdio_init_all();
    /* Test de base avant d'aller plus loin ;-) */
    // 1. Initialiser les sorties

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

#if 0

    int compteur = 0;

    // 2. Boucle infinie
    while (true) {
        printf("Blink numero : %d\n", compteur++);
        
        gpio_put(LED_PIN, 1); // Allumer
        sleep_ms(500);        // Attendre 500ms
        
        gpio_put(LED_PIN, 0); // Eteindre
        sleep_ms(500);        // Attendre 500ms
    }
#endif
    // Petite pause pour te laisser le temps d'ouvrir le moniteur série
    for (int i = 5; i > 0; i--)
    {
        printf("Demarrage dans %d...\n", i);

        gpio_put(LED_PIN, 1); // Allumer
        sleep_ms(500);        // Attendre 500ms

        gpio_put(LED_PIN, 0); // Eteindre
        sleep_ms(500);        // Attendre 500ms
    }

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
    case LORA_SEND:
        runForNodeMode();
        break;
    case LORA_RECEIVE:
        runForHubMode();
        break;
    default:
        printf("Mode non valide.\n");
        return -1;
    }

    return 0;
}