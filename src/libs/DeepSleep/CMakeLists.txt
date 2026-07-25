message("------- src/libs/DeepSleep/CMakeLists.txt ! -------")

# On crée la lib
add_library(src_lib_powerDownAndReboot STATIC 
    powerDownAndReboot.cpp
)

# On définit les chemins d'accès (le point '.' signifie 'ici même')
target_include_directories(src_lib_powerDownAndReboot PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# On lie les dépendances du SDK
target_link_libraries(src_lib_powerDownAndReboot 
    pico_stdlib 
    hardware_watchdog
    hardware_powman
    pico_rand
)