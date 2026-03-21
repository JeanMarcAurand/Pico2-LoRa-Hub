#include <stdio.h>
#include "pico/stdlib.h"

#include <MQTTLink.h>

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

const uint LED_HUB_PIN = 4;   // pour pico W, cabler la led!

int main()
{

    stdio_init_all();

    // Petite pause pour te laisser le temps d'ouvrir le moniteur série
 
    gpio_init(LED_HUB_PIN);
    gpio_set_dir(LED_HUB_PIN, GPIO_OUT);
    for (int i = 10; i > 0; i--)
    {
        printf("Demarrage dans %ds...  \r", i);

        gpio_put(LED_HUB_PIN, 1);  // Allumer
        sleep_ms(500);             // Attendre 500ms

        gpio_put(LED_HUB_PIN, 0);  // Eteindre
        sleep_ms(500);             // Attendre 500ms
    }

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
    if (status == MqttStatus::OK)
    {

        adc_init();
        adc_set_temp_sensor_enabled(true); // Active le capteur interne

        mqttLink.mqtt_subscribeTimeOut("pico/cmd", 10000);

        while (true)
        {
            char msg_buffer[32];
            float temp = read_internal_temperature();
            // Préparer le message texte
            snprintf(msg_buffer, sizeof(msg_buffer), "%.2f", temp);

            // Publier sur un nouveau topic
            MqttStatus status = mqttLink.mqtt_publishTimeOut("pico/sensors/temp", msg_buffer, 10000);
            if (status == MqttStatus::OK)
            {

                printf("Température envoyée : %s\n", msg_buffer);
                // Juste pour tester, on fait clignoter la LED du Wifi
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                mqttLink.active_wait(2500);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                mqttLink.active_wait(2500);
            }
            else
            {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                mqttLink.active_wait(100);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                mqttLink.active_wait(1000);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                mqttLink.active_wait(100);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                mqttLink.active_wait(1000);
            }
        }
    }
    else
    {
        while (true)
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            mqttLink.active_wait(100);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            mqttLink.active_wait(100);
        }
    }
    return 0;
}