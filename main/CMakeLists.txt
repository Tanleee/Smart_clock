idf_component_register(SRCS "src/main.c"
                            "src/display.cpp"
                            "src/wifi_sta.c"
                            "src/time_sync.c"
                            "src/alarm_data.c"
                       INCLUDE_DIRS "inc"
                       PRIV_REQUIRES esp_wifi esp_netif esp_event nvs_flash LovyanGFX)