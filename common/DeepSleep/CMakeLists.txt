# On crée la lib
add_library(powerDownAndReboot_common STATIC 
    powerDownAndReboot.cpp
)

# On définit les chemins d'accès (le point '.' signifie 'ici même')
target_include_directories(powerDownAndReboot_common PUBLIC 
    . 
)

# On lie les dépendances du SDK
target_link_libraries(powerDownAndReboot_common 
    pico_stdlib 
    hardware_watchdog
    hardware_powman
    pico_rand
)