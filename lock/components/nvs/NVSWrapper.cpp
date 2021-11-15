#include "NVSWrapper.h"

#define TAG "NVSWrapper"

NVSWrapper::NVSWrapper() { _handle = 0; }

NVSWrapper::~NVSWrapper() { close(); }

NVSWrapper* NVSWrapper::_instance = 0;

NVSWrapper* NVSWrapper::getInstance() { return _instance; }

void NVSWrapper::init() {
    if (_instance != 0) {
        ESP_LOGE(TAG, "NVSWrapper already initialized");
        return;
    }
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open("smartknockNVS", NVS_READWRITE, &_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }

    _instance = this;
}

bool NVSWrapper::exists(const char* key) {
    // Returns true if the key exists in NVS
    size_t required_size;
    esp_err_t err = nvs_get_blob(_handle, key, NULL, &required_size);
    if (err != ESP_OK) {
        printf("Error (%s) getting blob!\n", esp_err_to_name(err));
    }
    return (err == ESP_OK);
}

void NVSWrapper::erase(const char* key) {
    esp_err_t err = nvs_erase_key(_handle, key);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) erasing NVS key (%s)!", esp_err_to_name(err), key);
    }
}

void NVSWrapper::eraseAll() {
    esp_err_t err = nvs_erase_all(_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) erasing NVS keys!", esp_err_to_name(err));
    }
}

void NVSWrapper::commit() {
    esp_err_t err = nvs_commit(_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) committing NVS handle!", esp_err_to_name(err));
    }
}

void NVSWrapper::close() {
    if (_handle) {
        nvs_close(_handle);
        _handle = 0;
    }
}