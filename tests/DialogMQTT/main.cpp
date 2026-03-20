#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "hardware/adc.h"

#include "wifi_credentials.h"
void active_wait(uint32_t delay_ms)
{
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (to_ms_since_boot(get_absolute_time()) - start < delay_ms)
    {
        cyw43_arch_poll(); // On traite les paquets Wi-Fi/MQTT
        sleep_ms(1);       // On laisse un micro-dodo pour la conso
    }
}
mqtt_client_t *static_client;
static bool mqtt_is_connected = false;
static bool publish_in_progress = false;

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    if (status == MQTT_CONNECT_ACCEPTED)
    {
        printf("[mqtt_connection_cb] Connexion établie avec succès !\n");
        mqtt_is_connected = true;
    }
    else
    {
        printf("[mqtt_connection_cb] Déconnexion ou erreur : %d\n", status);
        mqtt_is_connected = false;
    }
}
// Callback quand le message est envoyé
static void mqtt_pub_request_cb(void *arg, err_t result)
{
    if (result != ERR_OK)
    {
        printf("[mqtt_pub_request_cb] Erreur de publication: %d\n", result);
        //        publish_in_progress = false;
    }
    else
    {
        printf("[mqtt_pub_request_cb] Message publié avec succès !\n");
        //        publish_in_progress = true;
    }
    publish_in_progress = false;
}

enum class MqttStatus
{
    OK,
    TIMEOUT,
    ERREUR
};

MqttStatus mqtt_connectTimeOut(ip_addr_t *broker_ip, int timeout_ms)
{
    MqttStatus returnValue = MqttStatus::ERREUR;
    static_client = mqtt_client_new();
    if (static_client != NULL)
    {
        struct mqtt_connect_client_info_t ci = {0};
        ci.client_id = "Pico2W_Hub";
        ci.keep_alive = 60;

        printf("[mqtt_connectTimeOut] IP du Broker : %s\n", ip4addr_ntoa(ip_2_ip4(broker_ip)));

        // Stocke le résultat de l'appel
        cyw43_arch_lwip_begin();
        err_t err = mqtt_client_connect(static_client, broker_ip, 1883, mqtt_connection_cb, NULL, &ci);
        cyw43_arch_lwip_end();

        if (err != ERR_OK)
        {
            printf("[mqtt_connectTimeOut] Erreur immédiate lors de l'appel connect : %d\n", err);
            returnValue = MqttStatus::ERREUR;
        }
        else
        {
            // Attend la fin de la connexion (callback mqtt_connection_cb)
            uint32_t start = to_ms_since_boot(get_absolute_time());
            while (mqtt_is_connected == false)
            {
                cyw43_arch_poll(); // Fait tourner la pile lwIP / Wi‑Fi
                sleep_ms(1);

                if (to_ms_since_boot(get_absolute_time()) - start > timeout_ms)
                {
                    printf("[mqtt_connectTimeOut] MQTT: delai de connexion depasse.\n");
                    break;
                }
            }
            if (mqtt_is_connected == true)
            {
                printf("[mqtt_connectTimeOut] Connexion établie avec succès aprés %dms!\n",
                       (to_ms_since_boot(get_absolute_time()) - start));
                returnValue = MqttStatus::OK;
            }
            else
            {
                printf("[mqtt_connectTimeOut] Pas de connection MQQT etablie aprés %dms !\n",
                       timeout_ms);
                returnValue = MqttStatus::TIMEOUT;
            }
        }
    }
    else
    {
        printf("[mqtt_connectTimeOut] ERREUR : Impossible de créer le client MQTT (mémoire ?)\n");
        returnValue = MqttStatus::ERREUR;
    }
    return (returnValue);
}

MqttStatus mqtt_publishTimeOut(const char *topic, const void *msg_buffer, int timeout_ms)
{
    MqttStatus returnValue = MqttStatus::ERREUR;

    // Vérifier si on est toujours connecté au niveau MQTT
    if (!mqtt_is_connected || static_client == NULL)
    {
        printf("[mqtt_publishTimeOut] Erreur : Client non connecté.\n");
        return MqttStatus::ERREUR;
    }

    if (publish_in_progress)
    {
        printf("[mqtt_publishTimeOut] Une publication est déjà en cours, on annule.\n");
        return MqttStatus::ERREUR;
    }

    printf("[mqtt_publishTimeOut] Tentative de publication sur %s avec la valeur : %s\n",
           topic, msg_buffer);
    publish_in_progress = true;
    cyw43_arch_lwip_begin(); // On bloque les interruptions réseau pour avoir l'exclusivité
    // Note : On utilise QoS 1 pour être SUR que le PC a reçu le message
    err_t status = mqtt_publish(static_client, topic, msg_buffer, strlen((const char *)msg_buffer), 1, 0, mqtt_pub_request_cb, NULL);
    cyw43_arch_lwip_end(); // On libère le réseau

    if (status == ERR_OK)
    {
        // Attend la fin de la publication (callback mqtt_pub_request_cb)
        uint32_t start = to_ms_since_boot(get_absolute_time());
        while (publish_in_progress == true)
        {
            cyw43_arch_poll(); // Fait tourner la pile lwIP / Wi‑Fi
            sleep_ms(1);

            if (to_ms_since_boot(get_absolute_time()) - start > timeout_ms)
            {
                printf("[mqtt_publishTimeOut] MQTT: delai de publication depasse.\n");
                break;
            }
        }

        if (publish_in_progress == false)
        {
            printf("[mqtt_publishTimeOut] Publication effectue avec succès aprés %dms!\n",
                   (to_ms_since_boot(get_absolute_time()) - start));
            returnValue = MqttStatus::OK;
        }
        else
        {
            printf("[mqtt_publishTimeOut] Pas de publication finie aprés %dms !\n",
                   timeout_ms);
            returnValue = MqttStatus::TIMEOUT;
        }
    }
    else
    {
        switch (status)
        {
        case ERR_CONN:
            printf("[mqtt_publishTimeOut] mqtt_publish erreur pas connecte!\n");
            break;
        case ERR_MEM:
            printf("[mqtt_publishTimeOut] mqtt_publish pas assez de memoire!\n");
            break;
        default:
            printf("[mqtt_publishTimeOut] mqtt_publish code retour inconnu = %d!\n", status);
            break;
        }
        publish_in_progress = false; // On libère si l'envoi a échoué tout de suite.
        returnValue = MqttStatus::ERREUR;
    }

    return (returnValue);
}

bool init_wifi()
{
    // 1. Initialisation hardware
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_FRANCE))
    {
        printf("[WiFi] Erreur d'initialisation hardware\n");
        return false;
    }

    // 2. Activation du mode Station
    cyw43_arch_enable_sta_mode();

    // 3. Tentative de connexion (BLOQUANTE)
    // Cette fonction gère elle-même le polling et l'attente DHCP en interne
    printf("[WiFi] Connexion à %s...\n", WIFI_SSID);
    // Boucle sans fin, de toute manière si pas de wifi, ne fait rien .
    int err;
    do
    {
        // On utilise un timeout de 30 secondes, ce qui est standard pour le DHCP
        err = cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASSWORD,
                                               CYW43_AUTH_WPA2_AES_PSK);

        if (err != 0)
        {
            printf("[WiFi] Échec de la connexion (Code erreur: %d)\n", err);
        }
    } while (err != 0);

    // 4. Force le mode Performance (Anti-latence)
    // À faire JUSTE APRÈS la connexion réussie
    cyw43_wifi_pm(&cyw43_state, CYW43_PERFORMANCE_PM);

    printf("[WiFi] Connecté ! IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));
    return true;
}

float read_internal_temperature()
{
    // Sélectionner l'entrée ADC du capteur de température (canal 4)
    adc_select_input(4);

    // Lire la valeur brute (0 à 4095)
    uint16_t raw = adc_read();

    // Convertir en tension (3.3V / 12 bits)
    const float conversion_factor = 3.3f / (1 << 12);
    float voltage = raw * conversion_factor;

    // Formule typique pour le Pico (à ajuster selon la datasheet du RP2350 si besoin)
    // Temp = 27 - (Voltage - 0.706) / 0.001721
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

int main()
{

    stdio_init_all();

    if (init_wifi())
    {
        printf("Init du wifi reussie!\n");
    }
    else
    {
        printf("Echec init du wifi !!\n");
    }

    // 1. Déclarer la structure
    ip_addr_t broker_ip;

    // 2. Convertir la chaîne en structure ip_addr_t
    // La fonction retourne 1 si l'adresse est valide, 0 sinon
    if (!ipaddr_aton("192.168.1.16", &broker_ip))
    {
        printf("Adresse IP du broker invalide !\n");
    }

    // 3. Passer le pointeur à ta fonction
    MqttStatus status = mqtt_connectTimeOut(&broker_ip, 10000);
    if (status == MqttStatus::OK)
    {

        adc_init();
        adc_set_temp_sensor_enabled(true); // Active le capteur interne

        while (true)
        {
            char msg_buffer[32];
            float temp = read_internal_temperature();
            // Préparer le message texte
            snprintf(msg_buffer, sizeof(msg_buffer), "%.2f", temp);

            // Publier sur un nouveau topic
            MqttStatus status = mqtt_publishTimeOut("pico/sensors/temp", msg_buffer, 10000);
            if (status == MqttStatus::OK)
            {

                printf("Température envoyée : %s\n", msg_buffer);
                // Juste pour tester, on fait clignoter la LED du Wifi
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                active_wait(2500);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                active_wait(2500);
            }
            else
            {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                active_wait(100);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                active_wait(1000);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                active_wait(100);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                active_wait(1000);
            }
        }
    }
    else
    {
        while (true)
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            active_wait(100);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            active_wait(100);
        }
    }
    return 0;
}