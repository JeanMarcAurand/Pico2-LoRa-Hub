#include <stdio.h>
#include <cstdio>

#include "pico/stdio.h"
#include "hardware/gpio.h"
#include "hardware/powman.h"

#include "powerDownAndReboot.h"
#include <pico/time.h>



void powerDownAndReboot_ms(uint32_t delay_ms, bool withJitter, bool debug)
{

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
    
   if (withJitter) {
        // Génère un delta entre -10% et +10%.
        int32_t range = delay_ms / 10; 
        if (range > 0) {
            int32_t offset = (int32_t)(get_rand_32() % (2 * range)) - range;
            delay_ms += offset;
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
        const uint LED_NODE_PIN = 25; // pour pico sans W, led intédrée.

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
