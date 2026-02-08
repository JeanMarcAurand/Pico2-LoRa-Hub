# On crée la lib
add_library(lora_common STATIC 
    LoRa.cpp
)

# On définit les chemins d'accès (le point '.' signifie 'ici même')
target_include_directories(lora_common PUBLIC 
    . 
)

# On lie les dépendances du SDK
target_link_libraries(lora_common 
    pico_stdlib 
    hardware_spi
)