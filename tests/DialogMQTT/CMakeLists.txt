message("------- tests/DialogMQTT/CMakeLists.txt ! -------")

# 1. Définir la carte AVANT tout le reste
set(PICO_BOARD pico2_w)

# 2. Configuration des sources et libs
set(DIALOG_MQTT_TEST_SOURCES main.cpp)
set(DIALOG_MQTT_TEST_LIBS 
    pico_stdlib 
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_adc
    pico_lwip_mqtt
    src_lib_mqttLink )


# 3. Créer l'exécutable
add_executable(test_dialogMqtt ${DIALOG_MQTT_TEST_SOURCES})

target_include_directories(test_dialogMqtt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 4. Activer l'USB (pour printf)
pico_enable_stdio_usb(test_dialogMqtt 1)
pico_enable_stdio_uart(test_dialogMqtt 0)

# 5. Lier les bibliothèques
target_link_libraries(test_dialogMqtt ${DIALOG_MQTT_TEST_LIBS})

# 6. Générer le fichier .uf2
pico_add_extra_outputs(test_dialogMqtt)

