# 1. On déclare toutes nos bibliothèques en premier
add_subdirectory(libs/TM1637Display)
add_subdirectory(libs/PersistantParams)
add_subdirectory(libs/DeepSleep)
add_subdirectory(libs/LoRa)
add_subdirectory(libs/LoRaLink)
add_subdirectory(libs/MQTTLink)
add_subdirectory(libs/messages)
add_subdirectory(libs/core)       # La HubFactory (dépend des libs ci-dessus)

# 2. Une fois que les libs existent, on peut compiler les applications finaux
add_subdirectory(apps/ClockTimerReveil)
add_subdirectory(apps/Hub)