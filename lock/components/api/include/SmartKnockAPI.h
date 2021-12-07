#include <string.h>

#include <sstream>
#include <string>

#include "esp_http_client.h"

enum MessageType { NONE, LOCK, UNLOCK };

struct ServerMessage {
    int id;
    MessageType type;
};

struct LockMessage {
    float battery;
    int knocks;
};

class SmartKnockAPI {
   public:
    static constexpr const char* api_base_url = "https://smartknock.xyz/";
    static constexpr const int http_response_max_size = 512;
    char* url = nullptr;
    uint32_t url_len = 0;
    MessageType get_incoming_message(const std::string &passphrase);
    void send_message(const std::string &passphrase, const LockMessage& msg);

    std::string get_unique_identifier(const std::string &passphrase);

   private:
    int last_consumed_message_id = 0;
    std::string compute_hash(const std::string& message);
    std::string get_mac_address();
    std::string make_http_get_request(const char* url);
    std::string make_http_post_request(const char* url, const char* data);
    static esp_err_t _http_event_handler(esp_http_client_event_t* evt);
};