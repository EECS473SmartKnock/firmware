idf_component_register(SRCS "SmartKnockAPI.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_client mbedtls nvs esp-tls)
