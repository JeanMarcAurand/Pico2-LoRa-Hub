message("------- src/apps/hub/CMakeLists.txt ! -------")

# Configuration des sources et libs
set(src_apps_hub_SOURCES main.cpp)
set(src_apps_hub_LIBS    pico_stdlib 
    pico_multicore 
    hardware_irq
    hardware_gpio
    # Attention à l'ordre!
    # 1. On met le cerveau en premier, car il exprime des besoins
    src_lib_core 
    # 2. On met ensuite les liaisons et la plomberie
    src_lib_mqttLink 
    src_lib_loraLink 
    src_lib_lora 
    src_lib_persistantParams 
    src_lib_powerDownAndReboot
    # 3. On met les messages en dernier, car TOUT LE MONDE pioche dedans
    src_lib_messages
    )


# Créer l'exécutable 
add_executable(src_apps_hub ${src_apps_hub_SOURCES})

# target_include_directories(src_apps_hub PRIVATE 
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${CMAKE_SOURCE_DIR}/src/libs/messages 
# )

# 4. Activer l'USB (pour printf)
pico_enable_stdio_usb(src_apps_hub 1)
pico_enable_stdio_uart(src_apps_hub 0)

# 5. Lier les bibliothèques
target_link_libraries(src_apps_hub ${src_apps_hub_LIBS})

# 6. Générer le fichier .uf2
pico_add_extra_outputs(src_apps_hub)

