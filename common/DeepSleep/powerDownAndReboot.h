#ifndef POWER_DOWN_AND_REBOOT_H
#define POWER_DOWN_AND_REBOOT_H

#include "pico/rand.h"
#include "hardware/watchdog.h"



void powerDownAndReboot_ms(uint32_t delay_ms, bool withJitter, bool debug);



#endif