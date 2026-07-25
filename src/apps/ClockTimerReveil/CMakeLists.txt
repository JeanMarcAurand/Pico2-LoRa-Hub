message("------- src/apps/CockTimerReveil/CMakeLists.txt ! -------")

# --- Configuration commune ---
set(src_apps_clockTimerReveilS_SOURCES main.cpp)
set(src_apps_clockTimerReveilS_LIBS 
    pico_stdlib 
    src_lib_lora src_lib_loraLink src_lib_messages
    src_lib_TM1637Display )
    
# --- L'exécutable NOEUD ---
add_executable(src_apps_clockTimerReveil ${src_apps_clockTimerReveilS_SOURCES})

# Pour voir tes printf via l'USB
pico_enable_stdio_usb(src_apps_clockTimerReveil 1)
pico_enable_stdio_uart(src_apps_clockTimerReveil 0)

# link avec les lib.
target_link_libraries(src_apps_clockTimerReveil ${src_apps_clockTimerReveilS_LIBS})

# Générer le .uf2 pour CET exécutable
pico_add_extra_outputs(src_apps_clockTimerReveil)

 