#include <stdio.h>

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string>

class NVSWrapper {
   public:
    NVSWrapper();
    ~NVSWrapper();
    void init();

    static NVSWrapper *getInstance();

    template <typename T>
    void set(const char *key, T value) {
        esp_err_t err = nvs_set_blob(_handle, key, &value, sizeof(T));
        if (err != ESP_OK) {
            printf("Error (%s) setting blob!\n", esp_err_to_name(err));
            return;
        }
    }
    template <typename T>
    T get(const char *key) {
        T value;
        size_t required_size = sizeof(T);
        esp_err_t err = nvs_get_blob(_handle, key, &value, &required_size);
        if (required_size != sizeof(T)) {
            printf("Error (%s) getting blob (length)! %s\n", esp_err_to_name(err), key);
        }
        if (err != ESP_OK) {
            printf("Error (%s) getting blob! %s\n", esp_err_to_name(err), key);
        }
        return value;
    }
    // Template specialization for std::string
    void set(const char *key, std::string value) {
        esp_err_t err = nvs_set_str(_handle, key, value.c_str());
        if (err != ESP_OK) {
            printf("Error (%s) setting string!\n", esp_err_to_name(err));
            return;
        }
    }
    std::string get(const char *key) {
        char value[128];
        size_t required_size = 128;
        esp_err_t err = nvs_get_str(_handle, key, value, &required_size);
        if (required_size > 128) {
            printf("Error (%s) getting string!\n", esp_err_to_name(err));
        }
        if (err != ESP_OK) {
            printf("Error (%s) getting string!\n", esp_err_to_name(err));
        }
        return std::string(value);
    }
    bool exists(const char *key);
    bool existsStr(const char *key);
    void erase(const char *key);
    void eraseAll();
    void commit();
    void close();

   private:
    nvs_handle _handle;
    static NVSWrapper *_instance;
};