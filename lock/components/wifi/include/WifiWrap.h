#include <string.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SSID_SIZE						32
#define MAX_PASSWORD_SIZE					64

struct WifiPassHeader {
    char* ssid;
    size_t ssid_len;

    char* password;
    size_t password_len;
};

class WifiWrap {
    public:
        WifiWrap();

        void connect(const WifiPassHeader& header);

        void disconnect();

        wifi_ap_record_t wifi_scan_networks(char* target_ssid, size_t target_ssid_len);

        void connection_check();
    private:
        static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
};

#ifdef __cplusplus
}
#endif