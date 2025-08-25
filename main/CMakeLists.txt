idf_component_register(
    SRCS 
        "main.c"
        "app_web.c"
        "rfid_reader.c"
        "rfid_storage.c"
    INCLUDE_DIRS 
        "."
        "${CMAKE_SOURCE_DIR}/managed_components/espressif__esp-zigbee-lib/include"
        "${CMAKE_SOURCE_DIR}/managed_components/espressif__esp-zigbee-lib/include/platform"
     REQUIRES
        esp-zigbee-lib
        esp-zboss-lib
        nvs_flash
        esp_http_server
        esp_netif
        esp_event
        driver
        esp_wifi 
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=implicit-function-declaration)
