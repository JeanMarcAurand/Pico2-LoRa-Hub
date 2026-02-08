# --- Configuration commune ---
set(LORA_TEST_SOURCES main.cpp)
set(LORA_TEST_LIBS lora_common pico_stdlib)

# --- 1. L'exécutable EMETTEUR ---
add_executable(test_lora_send ${LORA_TEST_SOURCES})

# Pour voir tes printf via l'USB
pico_enable_stdio_usb(test_lora_send 1)
pico_enable_stdio_uart(test_lora_send 0)

# Générer le .uf2 pour CET exécutable
pico_add_extra_outputs(test_lora_send)

# link avec les lib.
target_link_libraries(test_lora_send ${LORA_TEST_LIBS})

# On injecte la clé de compilation pour l'émetteur
target_compile_definitions(test_lora_send PRIVATE LORA_MODE_SEND)

# --- 1. L'exécutable RECEPTEUR ---
add_executable(test_lora_recv ${LORA_TEST_SOURCES})

# Pour voir tes printf via l'USB
pico_enable_stdio_usb(test_lora_recv 1)
pico_enable_stdio_uart(test_lora_recv 0)

# Générer le .uf2 pour CET exécutable
pico_add_extra_outputs(test_lora_recv)

# link avec les lib.
target_link_libraries(test_lora_recv ${LORA_TEST_LIBS})

# On injecte la clé de compilation pour l'émetteur
target_compile_definitions(test_lora_recv PRIVATE LORA_MODE_RECV)