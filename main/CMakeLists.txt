idf_component_register(SRCS "main.c" "display.c" "canvas.c" "font.c" "font_data.c" "ui.c" "player.c" "proto.c"
                            "encoder.c" "encoder_decode.c" "gesture.c" "wifi.c" "link.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_spi esp_driver_ledc esp_lcd esp_driver_gpio esp_wifi esp_netif nvs_flash esp_event
                                esp_http_server esp_timer cjson mdns)
