# Dans common/interface/CMakeLists.txt
message("------- COUCOU : JE SUIS DANS common/interface/CMakeLists.txt ! -------")

# 1. On crée la bibliothèque
add_library(interface_common_lib 
    SolarMeasureMsg.cpp
    ClockTimerMsg.cpp
    HubFactory.cpp
)

# 2. On définit les chemins d'inclusion (Attention à la parenthèse !)
target_include_directories(interface_common_lib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../ArduinoJson  # Utilise le chemin relatif propre
)

# 3. On lie les dépendances
target_link_libraries(interface_common_lib 
    pico_stdlib
)