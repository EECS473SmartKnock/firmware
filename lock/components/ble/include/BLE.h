#pragma once

#include <string>

#include "NimBLEDevice.h"
#include "freertos/FreeRTOS.h"

/*

    AppService
        This is the service used by the React Native mobile application for
        both initial setup as well as runtime configuration (to change WiFi networks,
   etc.)

        The WiFiSSID and WifiPassword characteristics are used to configure the WiFi
   network from the app. During the initial setup, the app will write these
   characteristics, and then will write the PassphraseCharacteristic with the desired
   passphrase. So the ESP32 is waiting for the onWrite event of the
   PassphraseCharacteristic at which point it configures the WiFi and the passphrase. From
   here on out, all WiFi configuration changes will be done by waiting for the
   HashCharacteristic's onWrite. The HashCharacteristic must have
   hash(ssid+password+passphrase) written to it for the new credentials to be considered
   valid. Additionally, there is a ResetCharacteristic that can be written to to reset the
   entire device (WiFi and passphrase) To do so, the data written to the
   ResetCharacteristic must be hash(ResetChallengeCharacterstic) where
   ResetChallengeCharacterstic is a random string that is updated intermittently by the
   ESP32

    FobService
        TODO

*/
static constexpr const char* const AppServiceUUID =
    "7fc9c7df-28b0-4177-ad55-60ba10b1d1dd";
static constexpr const char* const WifiSSIDCharacteristicUUID =
    "940c872e-2939-4162-81ae-648fa80382c4";
static constexpr const char* const WifiPasswordCharacteristicUUID =
    "940c872e-2939-4162-81ae-648fa80382c5";
static constexpr const char* const HashCharacteristicUUID =
    "940c872e-2939-4162-81ae-648fa80382c6";
static constexpr const char* const PassphraseCharacteristicUUID =
    "940c872e-2939-4162-81ae-648fa80382c7";
static constexpr const char* const ResetCharacteristicUUID =
    "940c872e-2939-4162-81ae-648fa80382c8";
static constexpr const char* const ResetChallengeCharacteristicUUID =
    "940c872e-2939-4162-81ae-648fa80382c9";
static constexpr const char* const MACCharacteristicUUID =
    "940c872e-2939-4162-81ae-648fa80382ca";

static constexpr const char* const FobServiceUUID =
    "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
static constexpr const char* const FobRxCharacteristicUUIDs[4] = {
    "beb5483e-36e1-4688-b7f5-ea07361b26a8", "beb5483e-36e1-4688-b7f5-ea07361b26a9",
    "beb5483e-36e1-4688-b7f5-ea07361b26aa", "beb5483e-36e1-4688-b7f5-ea07361b26ab"};
static constexpr const char* const FobTxCharacteristicUUIDs[4] = {
    "6389fb4f-d7a5-4a04-a416-6dfce3f0f14d", "6389fb4f-d7a5-4a04-a416-6dfce3f0f14e",
    "6389fb4f-d7a5-4a04-a416-6dfce3f0f14f", "6389fb4f-d7a5-4a04-a416-6dfce3f0f150"};

class BLE {
   public:
    BLE() : appCallbacks(this), scanCallbacks(this) {};
    void init();
    void deinit();
    void startServer();
    void stopServer();

    bool connectToFob();
    void disconnectFromFob();
    void setWifiSSID(const std::string& ssid);
    void setWifiPassword(const std::string& password);

    // Called when initial setup completes, returning the passphrase and WiFi credentials
    // Does NOT check if the device is actually in setup mode
    std::function<void(const std::string& passphrase, const std::string& ssid,
                       const std::string& password)>
        onSetupComplete;

    std::function<void()> onResetRequested;

    // Called during normal operation after an "authenticated" request to change WiFi
    // credentials
    std::function<void(const std::string& ssid, const std::string& password)>
        onWifiCredentialsUpdated;

    void fobWrite(const uint8_t* data);
    bool fobRecv(uint8_t* out);

    static constexpr const int fobScanTimeSec = 15;

   private:
    NimBLEServer* server = nullptr;
    NimBLEScan* scan = nullptr;
    NimBLEClient* client = nullptr;

    NimBLEService* appService = nullptr;
    NimBLECharacteristic* wifiSSIDCharacteristic = nullptr;
    NimBLECharacteristic* wifiPasswordCharacteristic = nullptr;
    NimBLECharacteristic* hashCharacteristic = nullptr;
    NimBLECharacteristic* passphraseCharacteristic = nullptr;
    NimBLECharacteristic* resetCharacteristic = nullptr;
    NimBLECharacteristic* resetChallengeCharacteristic = nullptr;
    NimBLECharacteristic* macCharacteristic = nullptr;
    NimBLERemoteService* fobService = nullptr;
    NimBLERemoteCharacteristic* fobTxCharacteristics[4] = {nullptr};
    NimBLERemoteCharacteristic* fobRxCharacteristics[4] = {nullptr};

    static NimBLEAdvertisedDevice* fobDevice;
    static BLE* instance;

    static uint8_t fobRecvFlag;
    static SemaphoreHandle_t fobScanSemaphore;
    static SemaphoreHandle_t fobRecvSemaphore;

    class AppCallbacks : public NimBLECharacteristicCallbacks {
       public:
        AppCallbacks(BLE* ble) : ble(ble) {}
        void onWrite(NimBLECharacteristic* pCharacteristic) override;
        // void onRead(NimBLECharacteristic* pCharacteristic) override;
        // void onNotify(NimBLECharacteristic* pCharacteristic) override;

       private:
        BLE* ble;
    };

    class ScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
       public:
        ScanCallbacks(BLE* ble) : ble(ble) {}
        void onResult(NimBLEAdvertisedDevice* advertisedDevice) override;

       private:
        BLE* ble;
    };

    static void fobNotify(NimBLERemoteCharacteristic* pRemoteCharacteristic,
                          uint8_t* pData, size_t length, bool isNotify);

    AppCallbacks appCallbacks;
    ScanCallbacks scanCallbacks;
};
