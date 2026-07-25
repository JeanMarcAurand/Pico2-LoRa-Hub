message("------- src/libs/messages/CMakeLists.txt ! -------")


# On s'assure que le SDK connaît la cible hardware_rtc
# Cela force CMake à aller chercher les headers au bon endroit
#find_package(pico_sdk REQUIRED)

# 1. On crée la bibliothèque
add_library(src_lib_messages 
    SolarMeasureMsg.cpp
    ClockTimerMsg.cpp
    CurrentTimeMsg.cpp
    BootRequestMsg.cpp
)

# 2. On définit les chemins d'inclusion (Attention à la parenthèse !)
target_include_directories(src_lib_messages PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/third_party/ArduinoJson  # Utilise le chemin relatif propre
)

# 3. On lie les dépendances
target_link_libraries(src_lib_messages 
    pico_stdlib
    pico_util        # Pour les structures de données
)
