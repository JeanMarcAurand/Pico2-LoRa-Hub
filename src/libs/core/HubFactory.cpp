#include "HubFactory.h"

#include "ClockTimerMsg.h"
#include "CurrentTimeMsg.h"
#include "BootRequestMsg.h"
#include "SolarMeasureMsg.h"

BaseMessage *HubFactory::memBaseMessage = nullptr;
BaseMessage *HubFactory::registry[static_cast<uint8_t>(LoRaMsgType::NB_OF_MSG)] = {nullptr};
char HubFactory::mqttStaticBuffer[MQTT_MAX_PAYLOAD_SIZE]={0};

uint16_t HubFactory::mqttBufferIndex = 0;

HubFactory::HubFactory()
{
    for (int i = 0; i < (int)LoRaMsgType::NB_OF_MSG; i++)
    {
        registry[i] = nullptr;
    }
}

void HubFactory::init(MQTTLink *mqttLink)
{
    registerMessage(ClockTimerMsg::getInstance(), mqttLink);
    registerMessage(CurrentTimeMsg::getInstance(), mqttLink);
    registerMessage(BootRequestMsg::getInstance(), mqttLink);
    registerMessage(SolarMeasureMsg::getInstance(), mqttLink);
}