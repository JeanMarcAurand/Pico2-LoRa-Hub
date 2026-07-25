#ifndef POWER_DOWN_AND_REBOOT_H
#define POWER_DOWN_AND_REBOOT_H

#include "pico/rand.h"
#include "hardware/watchdog.h"

class PowerDownAndReboot
{
private:
    /* data */
public:
    PowerDownAndReboot(/* args */);
    ~PowerDownAndReboot();

    void powerDownAndReboot_ms(uint32_t delay_ms, bool withJitter, bool debug);

};
#endif