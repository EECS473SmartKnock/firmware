idf_component_register(SRCS "BLE.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES esp-nimble-cpp nvs freertos mbedtls)
