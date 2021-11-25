#include "BLE.h"

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
    miscTxCharacteristic = appService->createCharacteristic(MiscTxCharacteristicUUID);
    miscRxCharacteristic = appService->createCharacteristic(MiscRxCharacteristicUUID);

    fobService = server->createService(FobServiceUUID);
    for (int i = 0; i < 4; i++) {
        fobTxCharacteristics[i] =
            fobService->createCharacteristic(FobTxCharacteristicUUIDs[i]);
        fobRxCharacteristics[i] =
            fobService->createCharacteristic(FobRxCharacteristicUUIDs[i]);
    }

    // Set callbacks for all characteristics
    wifiSSIDCharacteristic->setCallbacks(&appCallbacks);
    wifiPasswordCharacteristic->setCallbacks(&appCallbacks);
    miscTxCharacteristic->setCallbacks(&appCallbacks);
    miscRxCharacteristic->setCallbacks(&appCallbacks);

    for (int i = 0; i < 4; i++) {
        fobTxCharacteristics[i]->setCallbacks(&fobCallbacks);
        fobRxCharacteristics[i]->setCallbacks(&fobCallbacks);
    }

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
    if (pCharacteristic == ble->wifiSSIDCharacteristic ||
        pCharacteristic == ble->wifiPasswordCharacteristic) {
        ble->onWifiCredentialsUpdated(ble->wifiSSIDCharacteristic->getValue().c_str(),
                                 ble->wifiPasswordCharacteristic->getValue().c_str());
    }

    // TODO handle misc data
}

void BLE::FobCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    // TODO handle fob data
}