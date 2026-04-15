#include "HubFactory.h"

#include "ClockTimerMsg.h"
#include "SolarMeasureMsg.h"

BaseMessage *HubFactory::registry[static_cast<uint8_t>(LoRaMsgType::NB_OF_MSG)] = {nullptr};

HubFactory::HubFactory()
{
    for (int i = 0; i < (int)LoRaMsgType::NB_OF_MSG; i++)
    {
        registry[i] = nullptr;
    }
}

void HubFactory::init()
{
    registerMessage(ClockTimerMsg::getInstance());
    registerMessage(SolarMeasureMsg::getInstance());
}