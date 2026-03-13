# On crée la lib
add_library(persistantParams_common STATIC 
    PersistantParams.cpp
)

# On définit les chemins d'accès (le point '.' signifie 'ici même')
target_include_directories(persistantParams_common PUBLIC 
    . 
)

# On lie les dépendances du SDK
target_link_libraries(persistantParams_common 
    pico_stdlib 
    hardware_powman
)