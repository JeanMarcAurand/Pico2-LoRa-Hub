message("------- tests/TM1637/CMakeLists.txt ! -------")

# Configuration des sources et libs
set(TM1637_TEST_SOURCES main.cpp)
set(TM1637_TEST_LIBS pico_stdlib 
    hardware_gpio
    src_lib_TM1637Display 
 )


# Créer l'exécutable 
add_executable(test_TM1637 ${TM1637_TEST_SOURCES})

target_include_directories( test_TM1637 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )

# Activer l'USB (pour printf)
pico_enable_stdio_usb(test_TM1637 1)
pico_enable_stdio_uart(test_TM1637 0)

# Lier les bibliothèques
target_link_libraries(test_TM1637 ${TM1637_TEST_LIBS})

# Générer le fichier .uf2
pico_add_extra_outputs(test_TM1637)
