# --- Configuration commune ---
set(DIALOG_LORA_TEST_SOURCES main.cpp)
set(DIALOG_LORA_TEST_LIBS pico_stdlib pico_rand hardware_rcp hardware_powman hardware_timer lora_common loraLink_common persistantParams_common powerDownAndReboot_common)
# --- 1. L'exécutable NOEUD ---
add_executable(test_dialogLora_node ${DIALOG_LORA_TEST_SOURCES})

# Pour voir tes printf via l'USB
pico_enable_stdio_usb(test_dialogLora_node 1)
pico_enable_stdio_uart(test_dialogLora_node 0)

# Générer le .uf2 pour CET exécutable
pico_add_extra_outputs(test_dialogLora_node)

# link avec les lib.
target_link_libraries(test_dialogLora_node ${DIALOG_LORA_TEST_LIBS})

# On injecte la clé de compilation pour l'émetteur
target_compile_definitions(test_dialogLora_node PRIVATE DIALOG_LORA_MODE_NODE)

# --- 2. L'exécutable HUB ---
add_executable(test_dialoglora_hub ${DIALOG_LORA_TEST_SOURCES})

# Pour voir tes printf via l'USB
pico_enable_stdio_usb(test_dialoglora_hub 1)
pico_enable_stdio_uart(test_dialoglora_hub 0)

# Générer le .uf2 pour CET exécutable
pico_add_extra_outputs(test_dialoglora_hub)

# link avec les lib.
target_link_libraries(test_dialoglora_hub ${DIALOG_LORA_TEST_LIBS})

# On injecte la clé de compilation pour l'émetteur
target_compile_definitions(test_dialoglora_hub PRIVATE DIALOG_LORA_MODE_HUB)