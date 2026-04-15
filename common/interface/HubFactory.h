#ifndef HUB_FACTORY_H
#define HUB_FACTORY_H

#include "BaseMessage.h"
#include "protocoleLora.h"

class HubFactory
{
public:
    HubFactory();

    void init();

    BaseMessage *getInstanceMessage(LoRaMsgType id)
    {
        uint8_t ident = static_cast<uint8_t>(id);
        if ((ident < static_cast<uint8_t>(LoRaMsgType::NB_OF_MSG)) &&
            (ident >= 0))
        {
            return (registry[ident]);
        }
        else
        {
            printf("ERREUR : ID de message %d hors limites !\n", ident);
            return (nullptr);
        }
    }

private:
    // Le tableau de pointeurs vers la classe mère
    static BaseMessage *registry[static_cast<uint8_t>(LoRaMsgType::NB_OF_MSG)];

    void registerMessage(BaseMessage *baseMessage)
    {
        registry[static_cast<uint8_t>(baseMessage->getMessageTypeId())] = baseMessage;
    }
};

#endif