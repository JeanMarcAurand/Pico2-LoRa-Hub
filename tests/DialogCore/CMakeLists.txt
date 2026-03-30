
# Configuration des sources et libs
set(DIALOG_CORE_TEST_SOURCES main.cpp)
set(DIALOG_CORE_TEST_LIBS    pico_stdlib pico_multicore interface_common_lib)


# Créer l'exécutable 
add_executable(test_dialogCore ${DIALOG_CORE_TEST_SOURCES})

target_include_directories(test_dialogCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
../interface )

# 4. Activer l'USB (pour printf)
pico_enable_stdio_usb(test_dialogCore 1)
pico_enable_stdio_uart(test_dialogCore 0)

# 5. Lier les bibliothèques
target_link_libraries(test_dialogCore ${DIALOG_CORE_TEST_LIBS})

# 6. Générer le fichier .uf2
pico_add_extra_outputs(test_dialogCore)

