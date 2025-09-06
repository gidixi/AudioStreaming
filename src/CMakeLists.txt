idf_component_register(
  SRCS
    "app_main.cpp"
    "client_main.cpp"
    "transport_udp.cpp"
    "opus_codec.cpp"
    "audio_i2s.cpp"
  INCLUDE_DIRS
    "../include"
  REQUIRES
    esp-libopus     # <-- questo è il nome del componente
    esp_driver_i2s
    esp_wifi
    esp_netif
    nvs_flash
    lwip
    log
)

target_compile_definitions(${COMPONENT_TARGET} PUBLIC PLATFORM_ESP32)
