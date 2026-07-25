message("------- src/libs/core/CMakeLists.txt ! -------")


# 1. Définir la carte AVANT tout le reste
set(PICO_BOARD pico2_w)

# 2. On crée la bibliothèque
add_library(src_lib_core STATIC
    HubFactory.cpp
)

# 3. On définit les chemins d'inclusion (Attention à la parenthèse !)
target_include_directories(src_lib_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/third_party/ArduinoJson  # Utilise le chemin relatif propre
    ${CMAKE_SOURCE_DIR}/src/libs/messages  # Utilise le chemin relatif propre
    ${CMAKE_SOURCE_DIR}/src/libs/MQTTLink  # Utilise le chemin relatif propre
)

# 4. On lie les dépendances
target_link_libraries(src_lib_core 
    pico_stdlib
    pico_util        # Pour les structures de données
    pico_multicore 
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_mqtt
)
