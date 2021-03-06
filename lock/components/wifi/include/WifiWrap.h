#pragma once

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
    const char* ssid;
    size_t ssid_len;

    const char* password;
    size_t password_len;
};

class WifiWrap {
    public:
        WifiWrap();

        void connect(const WifiPassHeader& header);

        void disconnect();

        void wifi_scan_networks(char* target_ssid);

        void connection_check();
    private:
        esp_netif_t *sta_netif_;
        esp_event_handler_instance_t instance_any_id_;
        esp_event_handler_instance_t instance_got_ip_;
        static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
};

#ifdef __cplusplus
}
#endif