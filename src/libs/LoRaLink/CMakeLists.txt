message("------- src/libs/LoRaLink/CMakeLists.txt ! -------")

set(LORA_LINK_LIBS pico_stdlib pico_rand hardware_rcp  hardware_timer src_lib_lora src_lib_persistantParams)


# On crée la lib
add_library(src_lib_loraLink STATIC 
    LoRaLink.cpp
)

# On définit les chemins d'accès (le point '.' signifie 'ici même')
target_include_directories(src_lib_loraLink PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/libs/messages
)

# On lie les dépendances du SDK
target_link_libraries(src_lib_loraLink ${LORA_LINK_LIBS} )
