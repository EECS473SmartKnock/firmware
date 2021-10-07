#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SSID_SIZE						32
#define MAX_PASSWORD_SIZE					64

class WifiWrap {
    public:
        WifiWrap();

        void connect();

        esp_err_t disconnect();
    private:
        static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
};

#ifdef __cplusplus
}
#endif