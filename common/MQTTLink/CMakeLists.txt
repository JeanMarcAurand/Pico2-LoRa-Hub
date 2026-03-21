# 1. Définir la carte AVANT tout le reste
set(PICO_BOARD pico2_w)

# 2. Configuration des libs
set(MQTT_LINK_LIBS 
    pico_stdlib 
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_adc
    pico_lwip_mqtt)


# On crée la lib
add_library(mqttLink_common STATIC 
    MQTTLink.cpp
)
# On définit les chemins d'accès (le point '.' signifie 'ici même')
target_include_directories(mqttLink_common PUBLIC 
    .
)

# On lie les dépendances du SDK
target_link_libraries(mqttLink_common ${MQTT_LINK_LIBS} )

