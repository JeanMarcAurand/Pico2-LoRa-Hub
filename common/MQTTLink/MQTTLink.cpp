#include "MQTTLink.h"

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

// --- 1. Callback appelée quand un message arrive sur un topic abonné ---
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    char message[128];
    u16_t scan_len = (len < 127) ? len : 127;
    memcpy(message, data, scan_len);
    message[scan_len] = '\0'; // On s'assure que c'est une chaîne de caractères

    printf("[mqtt_incoming_data_cb] Message reçu : %s\n", message);

    // Exemple d'action simple :
    if (strcmp(message, "ON") == 0)
    {
        printf("[mqtt_incoming_data_cb] On allume la LED !\n");
        // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    }
}

// --- 2. Callback appelée quand on reçoit le contenu d'un topic ---
// Note: lwIP sépare l'arrivée du NOM du topic et la DONNÉE.
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    printf("[mqtt_incoming_publish_cb] Nouveau message sur le topic : %s (taille: %u)\n", topic, (unsigned int)tot_len);
}

// --- 3. Callback de confirmation d'abonnement ---
static bool mqtt_sub_ready = false; // Flag de confirmation

// La callback de confirmation
static void mqtt_sub_request_cb(void *arg, err_t result)
{
    if (result == ERR_OK)
    {
        printf("[mqtt_sub_request_cb] Confirmation SUBACK reçue : Abonnement actif.\n");
        mqtt_sub_ready = true; // On lève le drapeau
    }
    else
    {
        printf("[mqtt_sub_request_cb] Échec confirmation abonnement : %d\n", result);
    }
}

MQTTLink::MQTTLink()
{
    init_wifi();
}

MqttStatus MQTTLink::mqtt_connectTimeOut(ip_addr_t *broker_ip, int timeout_ms)
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

MqttStatus MQTTLink::mqtt_publishTimeOut(const char *topic, const void *msg_buffer, int timeout_ms)
{
    MqttStatus returnValue = MqttStatus::ERREUR;

    // Vérifier si on est toujours connecté au niveau MQTT.
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

MqttStatus MQTTLink::mqtt_subscribeTimeOut(const char *topic, int timeout_ms)
{

    MqttStatus returnValue = MqttStatus::ERREUR;
    mqtt_sub_ready = false; // On reset avant de commencer

    // 1. Configurer les callbacks de réception (Indispensable avant de s'abonner).
    mqtt_set_inpub_callback(static_client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, NULL);

    // 2. Envoyer la demande
    cyw43_arch_lwip_begin();
    err_t err = mqtt_sub_unsub(static_client, topic, 1, mqtt_sub_request_cb, NULL, 1);
    cyw43_arch_lwip_end();

    if (err != ERR_OK)
    {
        printf("[mqtt_subscribeTimeOut] Erreur immédiate lors de l'appel connect : %d\n", err);
        returnValue = MqttStatus::ERREUR;
    }
    else
    {
        // 3. ATTENDRE la confirmation du Broker (SUBACK)
        uint32_t start = to_ms_since_boot(get_absolute_time());
        while (!mqtt_sub_ready)
        {
            cyw43_arch_poll(); // On traite les paquets entrants (le SUBACK arrive ici !)
            sleep_ms(1);
            if (to_ms_since_boot(get_absolute_time()) - start > timeout_ms)
            {
                printf("[mqtt_subscribeTimeOut] Timeout : La confirmation de l'abonnement par le Broker n'a pas été recu!\n");
                break;
            }
        }
        if (mqtt_sub_ready == true)
        {
            printf("[mqtt_subscribeTimeOut] Abonnement pour %s fait avec succès aprés %dms!\n",
                   topic,
                   (to_ms_since_boot(get_absolute_time()) - start));
            returnValue = MqttStatus::OK;
        }
        else
        {
            printf("[mqtt_connectTimeOut] Pas de confimation de l'abonnement à %s aprés %dms !\n",
                   topic,
                   timeout_ms);
            returnValue = MqttStatus::TIMEOUT;
        }
    }
    return (returnValue);
}

bool MQTTLink::init_wifi()
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
void MQTTLink::active_wait(uint32_t delay_ms)
{
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (to_ms_since_boot(get_absolute_time()) - start < delay_ms)
    {
        cyw43_arch_poll(); // On traite les paquets Wi-Fi/MQTT
        sleep_ms(1);       // On laisse un micro-dodo pour la conso
    }
}