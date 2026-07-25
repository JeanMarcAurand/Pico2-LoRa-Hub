message("------- src/libs/LoRa/CMakeLists.txt ! -------")

# On crée la lib
add_library(src_lib_lora STATIC 
    LoRa.cpp
)

# On définit les chemins d'accès (le point '.' signifie 'ici même')
target_include_directories(src_lib_lora PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# On lie les dépendances du SDK
target_link_libraries(src_lib_lora 
    pico_stdlib 
    hardware_spi
)