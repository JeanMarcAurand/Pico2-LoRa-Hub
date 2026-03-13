set(LORA_LINK_LIBS pico_stdlib pico_rand hardware_rcp  hardware_timer lora_common persistantParams_common)


# On crée la lib
add_library(loraLink_common STATIC 
    LoRaLink.cpp
)

# On définit les chemins d'accès (le point '.' signifie 'ici même')
target_include_directories(loraLink_common PUBLIC 
    . ../interface
)

# On lie les dépendances du SDK
target_link_libraries(loraLink_common ${LORA_LINK_LIBS} )
