
message("------- src/libs/TM1637Display/CMakeLists.txt ! -------")

# On crée la lib
add_library(src_lib_TM1637Display STATIC 
    TM1637Display.cpp
)

# On définit les chemins d'accès (le point '.' signifie 'ici même')
target_include_directories(src_lib_TM1637Display PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

# On lie les dépendances du SDK
target_link_libraries(src_lib_TM1637Display 
    pico_stdlib 
    hardware_gpio
)