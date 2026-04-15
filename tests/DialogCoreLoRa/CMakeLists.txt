
# Configuration des sources et libs
set(DIALOG_CORE_LORA_TEST_SOURCES main.cpp)
set(DIALOG_CORE_LORA_TEST_LIBS    pico_stdlib 
    pico_multicore 
    hardware_irq
    hardware_gpio
    interface_common_lib 
    lora_common loraLink_common 
    mqttLink_common 
    persistantParams_common powerDownAndReboot_common)


# Créer l'exécutable 
add_executable(test_dialogCoreLoRa ${DIALOG_CORE_LORA_TEST_SOURCES})

target_include_directories(test_dialogCoreLoRa PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
../interface )

# 4. Activer l'USB (pour printf)
pico_enable_stdio_usb(test_dialogCoreLoRa 1)
pico_enable_stdio_uart(test_dialogCoreLoRa 0)

# 5. Lier les bibliothèques
target_link_libraries(test_dialogCoreLoRa ${DIALOG_CORE_LORA_TEST_LIBS})

# 6. Générer le fichier .uf2
pico_add_extra_outputs(test_dialogCoreLoRa)
