idf_component_register(SRCS "DeepSleep.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES wifi ble esp_adc ulp_adc)