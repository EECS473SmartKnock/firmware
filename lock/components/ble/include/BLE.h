#include "NimBLEDevice.h"

static constexpr const char* const AppServiceUUID = "7fc9c7df-28b0-4177-ad55-60ba10b1d1dd";
static constexpr const char* const WifiSSIDCharacteristicUUID =
    "940c872e-2939-4162-81ae-648fa80382c4";
static constexpr const char* const WifiPasswordCharacteristicUUID =
    "940c872e-2939-4162-81ae-648fa80382c5";
static constexpr const char* const MiscTxCharacteristicUUID =
    "940c872e-2939-4162-81ae-648fa80382c6";
static constexpr const char* const MiscRxCharacteristicUUID =
    "940c872e-2939-4162-81ae-648fa80382c7";

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
    BLE() : appCallbacks(this), fobCallbacks(this){};
    void init();

    void setWifiSSID(const std::string& ssid);
    void setWifiPassword(const std::string& password);

    std::function<void(const char* ssid, const char* password)> onWifiCredentialsUpdated;

    void fobWrite(const char* data, uint8_t len);
    void fobRecv(char* data, uint8_t len);

   private:
    NimBLEServer* server = nullptr;
    NimBLEService* appService = nullptr;
    NimBLECharacteristic* wifiSSIDCharacteristic = nullptr;
    NimBLECharacteristic* wifiPasswordCharacteristic = nullptr;
    NimBLECharacteristic* miscTxCharacteristic = nullptr;
    NimBLECharacteristic* miscRxCharacteristic = nullptr;
    NimBLEService* fobService = nullptr;
    NimBLECharacteristic* fobTxCharacteristics[4] = {nullptr};
    NimBLECharacteristic* fobRxCharacteristics[4] = {nullptr};

    class AppCallbacks : public NimBLECharacteristicCallbacks {
       public:
        AppCallbacks(BLE* ble) : ble(ble) {}
        void onWrite(NimBLECharacteristic* pCharacteristic) override;
        // void onRead(NimBLECharacteristic* pCharacteristic) override;
        // void onNotify(NimBLECharacteristic* pCharacteristic) override;

       private:
        BLE* ble;
    };

    class FobCallbacks : public NimBLECharacteristicCallbacks {
       public:
        FobCallbacks(BLE* ble) : ble(ble) {}
        void onWrite(NimBLECharacteristic* pCharacteristic) override;
        // void onRead(NimBLECharacteristic* pCharacteristic) override;
        // void onNotify(NimBLECharacteristic* pCharacteristic) override;

       private:
        BLE* ble;
    };

    AppCallbacks appCallbacks;
    FobCallbacks fobCallbacks;
};
