idf_component_register(SRCS "src/main.c"
                            "src/display.c"
                            "src/wifi_sta.c"
                            "src/time_sync.c"
                       INCLUDE_DIRS "inc"
                       PRIV_REQUIRES esp_wifi esp_netif esp_event nvs_flash)