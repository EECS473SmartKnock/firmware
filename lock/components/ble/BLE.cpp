#include "BLE.h"

#include "NVSWrapper.h"
#include "esp_netif.h"
#include "mbedtls/sha256.h"

void BLE::init() {
    static bool initialized = false;
    if (initialized) {
        ESP_LOGE("BLE", "BLE already initialized");
        return;
    }
    initialized = true;
    NimBLEDevice::init("SmartKnock");

    server = NimBLEDevice::createServer();
    appService = server->createService(AppServiceUUID);
    wifiSSIDCharacteristic = appService->createCharacteristic(WifiSSIDCharacteristicUUID);
    wifiPasswordCharacteristic =
        appService->createCharacteristic(WifiPasswordCharacteristicUUID);
    hashCharacteristic = appService->createCharacteristic(HashCharacteristicUUID);
    passphraseCharacteristic =
        appService->createCharacteristic(PassphraseCharacteristicUUID);
    resetCharacteristic = appService->createCharacteristic(ResetCharacteristicUUID);
    resetChallengeCharacteristic =
        appService->createCharacteristic(ResetChallengeCharacteristicUUID);
    macCharacteristic = appService->createCharacteristic(MACCharacteristicUUID);

    fobService = server->createService(FobServiceUUID);
    for (int i = 0; i < 4; i++) {
        fobTxCharacteristics[i] =
            fobService->createCharacteristic(FobTxCharacteristicUUIDs[i]);
        fobRxCharacteristics[i] =
            fobService->createCharacteristic(FobRxCharacteristicUUIDs[i]);
    }

    // Set callbacks
    passphraseCharacteristic->setCallbacks(&appCallbacks);
    hashCharacteristic->setCallbacks(&appCallbacks);
    resetCharacteristic->setCallbacks(&appCallbacks);

    for (int i = 0; i < 4; i++) {
        fobRxCharacteristics[i]->setCallbacks(&fobCallbacks);
    }

    // Set MAC address
    auto get_mac_address = []() {
        uint8_t mac_address[6];
        char hex_address[64];
        ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac_address));
        for (int i = 0; i < 6; i++) sprintf(&hex_address[i * 2], "%02x", mac_address[i]);
        return std::string(hex_address);
    };
    macCharacteristic->setValue(get_mac_address());

    // Set ResetChallenge data to random value
    uint8_t resetChallengeData[6];
    esp_fill_random(resetChallengeData, sizeof(resetChallengeData));
    char hex_resetChallenge[64];
    for (int i = 0; i < 6; i++) sprintf(&hex_resetChallenge[i * 2], "%02x", resetChallengeData[i]);
    resetChallengeCharacteristic->setValue(std::string(hex_resetChallenge));

    // Start all services
    appService->start();
    fobService->start();

    // Advertise services
    server->getAdvertising()->addServiceUUID(appService->getUUID());
    server->getAdvertising()->addServiceUUID(FobServiceUUID);

    server->getAdvertising()->start();
    server->start();
}

void BLE::setWifiSSID(const std::string& ssid) {
    wifiSSIDCharacteristic->setValue(ssid);
    // wifiSSIDCharacteristic->notify();
}

void BLE::setWifiPassword(const std::string& password) {
    wifiPasswordCharacteristic->setValue(password);
    // wifiPasswordCharacteristic->notify();
}

void BLE::AppCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    auto compute_hash = [](const std::string& data) {
        char hash_buffer[64];
        mbedtls_sha256_context sha256_ctx;
        mbedtls_sha256_init(&sha256_ctx);
        mbedtls_sha256_starts(&sha256_ctx, false);
        mbedtls_sha256_update(&sha256_ctx, (const unsigned char*)data.c_str(),
                              data.length());
        mbedtls_sha256_finish(&sha256_ctx, (unsigned char*)hash_buffer);
        mbedtls_sha256_free(&sha256_ctx);
        // Convert to hex
        char hex_hash[64];
        for (int i = 0; i < 32; i++) sprintf(&hex_hash[i * 2], "%02x", hash_buffer[i]);
        return std::string(hex_hash);
    };

    if (pCharacteristic == ble->passphraseCharacteristic) {
        // WiFi credentials and passphrase were set in setup phase
        std::string passphrase = pCharacteristic->getValue();
        std::string ssid = ble->wifiSSIDCharacteristic->getValue();
        std::string password = ble->wifiPasswordCharacteristic->getValue();
        ble->onSetupComplete(passphrase, ssid, password);
    } else if (pCharacteristic == ble->hashCharacteristic) {
        // WiFi credentials and verification hash were set
        std::string passphrase = NVSWrapper::getInstance()->get("passphrase");

        std::string ssid = ble->wifiSSIDCharacteristic->getValue();
        std::string password = ble->wifiPasswordCharacteristic->getValue();
        std::string expected_hash = compute_hash(ssid + password + passphrase);
        std::string actual_hash = pCharacteristic->getValue();
        if (expected_hash != actual_hash) {
            ESP_LOGE("BLE", "WiFi update hash mismatch: %s, %s (%s, %s, %s)", expected_hash.c_str(),
                     actual_hash.c_str(), ssid.c_str(), password.c_str(), passphrase.c_str());
            return;
        }
        ble->onWifiCredentialsUpdated(ssid, password);
    }
    else if (pCharacteristic == ble->resetCharacteristic) {
        // Make sure reset hash is correct
        std::string passphrase = NVSWrapper::getInstance()->get("passphrase");
        std::string reset_challenge = ble->resetChallengeCharacteristic->getValue();
        std::string reset_hash = pCharacteristic->getValue();
        std::string expected_hash = compute_hash(reset_challenge + passphrase);
        if (expected_hash != reset_hash) {
            ESP_LOGE("BLE", "Reset hash mismatch: %s, %s, (%s, %s)", expected_hash.c_str(),
                     reset_hash.c_str(), reset_challenge.c_str(), passphrase.c_str());
            return;
        }
        ble->onResetRequested();
    }
}

void BLE::FobCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    // TODO handle fob data
    // TODO: I just realized that the original code posted by Ben in the Discord had the
    // ESP32 configured as a BLE Client. Here we need to have it be a Server because the
    // React Native library doesn't support acting as a server.

    // So we might need to change this to be a client, or change the code on the fob.

    if (!fobRecvSemaphore) return;

    int idx;
    if (pCharacteristic == ble->fobRxCharacteristics[0]) {
        idx = 0;
    } else if (pCharacteristic == ble->fobRxCharacteristics[1]) {
        idx = 1;
    } else if (pCharacteristic == ble->fobRxCharacteristics[2]) {
        idx = 2;
    } else if (pCharacteristic == ble->fobRxCharacteristics[3]) {
        idx = 3;
    } else {
        return;
    }

    // Make sure length of data is 16
    if (pCharacteristic->getValue().length() != 16) {
        ESP_LOGE("BLE", "Invalid data length");
        return;
    }

    fobRecvFlag |= 1 << idx;

    // Notify fobRecv semaphore
    xSemaphoreGive(ble->fobRecvSemaphore);
}

void BLE::fobWrite(const char* data) {
    for (int i = 0; i < 4; i++) {
        fobRxCharacteristics[i]->setValue((const uint8_t*)(data + (i * 16)), 16);
    }
}

SemaphoreHandle_t BLE::fobRecvSemaphore = nullptr;
uint8_t BLE::fobRecvFlag = 0;

bool BLE::fobRecv(char* out) {
    fobRecvSemaphore = xSemaphoreCreateBinary();
    fobRecvFlag = 0;
    // Block until all rx characteristics have been read
    while (fobRecvFlag != 0xF) {
        xSemaphoreTake(fobRecvSemaphore, portMAX_DELAY);
    }
    fobRecvSemaphore = nullptr;

    // Copy data to out
    for (int i = 0; i < 4; i++) {
        if (fobRxCharacteristics[i]->getValue().length() != 16) {
            ESP_LOGE("BLE", "Invalid data length");
            return false;
        }
        memcpy(out + (i * 16), fobRxCharacteristics[i]->getValue().c_str(), 16);
    }

    return true;
}