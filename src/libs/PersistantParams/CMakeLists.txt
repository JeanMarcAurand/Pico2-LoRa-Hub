message("------- src/libs/PersistantParams/CMakeLists.txt ! -------")

# On crée la lib
add_library(src_lib_persistantParams STATIC 
    PersistantParams.cpp
)

# On définit les chemins d'accès (le point '.' signifie 'ici même')
target_include_directories(src_lib_persistantParams PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# On lie les dépendances du SDK
target_link_libraries(src_lib_persistantParams 
    pico_stdlib 
    hardware_powman
)